#include <libgimp/gimp.h>

enum DIR
{
    DIR_HORIZONTAL,
    DIR_VERTICAL
};
static void query (void);
static void run (const gchar      *name,
                 gint              nparams,
                 const GimpParam  *param,
                 gint             *nreturn_vals,
                 GimpParam       **return_vals);
 static void getInitialCrop (GimpDrawable *drawable,
                              gint *c1,
                              gint *c2,
                              enum DIR direction);
static gboolean is_white (guchar *buf,
                         gint channels,
                         gint offset);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,
    NULL,
    query,
    run,
};

MAIN()

static void
query (void)
{
    static GimpParamDef args[] = {
        {
            GIMP_PDB_INT32,
            "run-mode",
            "Run Mode"
        },
        {
            GIMP_PDB_IMAGE,
            "image",
            "Input Image"
        },
        {
            GIMP_PDB_DRAWABLE,
            "drawable",
            "Input drawable"
        }
    };
    
    gimp_install_procedure (
        "plug-in-makeafishcropper",
        "Make A Fish Cropper",
        "Crops and removed the background to fish from makea.fish",
        "bobjrsenior",
        "Copyright info",
        "2022",
        "_Make A Fish Cropper",
        "RGB*",
        GIMP_PLUGIN,
        G_N_ELEMENTS (args), 0,
        args, NULL);
        
        gimp_plugin_menu_register ("plug-in-makeafishcropper", "<Image>/Filters/Misc");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
    static GimpParam   values[1];
    GimpPDBStatusType  status = GIMP_PDB_SUCCESS; 
    GimpRunMode        run_mode;
    GimpDrawable      *drawable;
    gint32             image_id;
    gint32             cx1, cy1, cx2, cy2;
    gboolean           result, failed;
    
    // Getting run_mode - we won't display a dialog if
    // we are in NONINTERACTIVE mode
    run_mode = param[0].data.d_int32;
    
    // Get the specified Image
    image_id = param[1].data.d_image;
    
    // Get the specified drawable
    drawable = gimp_drawable_get (param[2].data.d_drawable);
    
    gimp_progress_init ("Cropping the fish...");
    
    // Get the coordinates of the white fish borders
    getInitialCrop (drawable, &cy1, &cy2, DIR_VERTICAL);
    gimp_progress_update (25);
    getInitialCrop (drawable, &cx1, &cx2, DIR_HORIZONTAL);
    gimp_progress_update (50);
    
    g_message ("X: (%d, %d), Y: (%d, %d)\n", cx1, cx2, cy1, cy2);
    // Crop out the borders
    result = gimp_image_crop (image_id,
                              (cx2 - cx1), (cy2 - cy1),
                              cx1, cy1);
    
    gimp_progress_update (60);
    
    if (!result)
    {
        failed = TRUE;
        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            g_message ("Failed to crop out borders");
    }
    else
    {
        // Add an Alpha channel

        // Check if an Alpha channel already exists
        if (!gimp_drawable_has_alpha(drawable->drawable_id))
        {
            // The method to add an Alpha channel is at the layer level
            // So make sure we are running on a layer
            if (gimp_item_is_layer(drawable->drawable_id))
            {
                result = gimp_layer_add_alpha (drawable->drawable_id);
            }
        }
        else
        {
            // It already has an Alpha channel
            result = TRUE;
        }
    }
    
    gimp_progress_update (65);
    
    if (!result && !failed)
    {
        failed = TRUE;
        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            g_message ("Failed to add an Alpha channel");
    }
    else
    {
        // Select the background by color
        
        // Get new drawable bounds
        gint x1, y1, x2, y2;
        gimp_drawable_mask_bounds (drawable->drawable_id,
                                   &x1, &y1,
                                   &x2, &y2);
    
        // Save existing setting and configure selection settings
        gdouble prev_threshold = gimp_context_get_sample_threshold ();
        gimp_context_set_sample_threshold ( 0.2 );  //0.4
    
        // Select slightly offset from the upper left corner
        result = gimp_image_select_contiguous_color (image_id,
                                                     GIMP_CHANNEL_OP_REPLACE,
                                                     drawable->drawable_id,
                                                     (gdouble) (x1 + 4),
                                                     (gdouble) (y1 + 4));
        // Restore selection settings
        gimp_context_set_sample_threshold (prev_threshold);
    }
    
    gimp_progress_update (80);

    if (!result && !failed)
    {
        failed = TRUE;
        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            g_message ("Failed to select the background");
    }
    else
    {
        // Clear the selection (remove the background) 
        result = gimp_edit_clear (drawable->drawable_id);
    }

    gimp_progress_update (90);
    

    if (!result && !failed)
    {
        failed = TRUE;
        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            g_message ("Failed to clear the background");
    }
    else
    {
        // Finally Resize Image to what's left
        
        // Use Alpha selection to select what's left (just the fish)
        result = gimp_image_select_item (image_id,
                                         GIMP_CHANNEL_OP_REPLACE,
                                         drawable->drawable_id);
        
        if (result)
        {
            // Get the selection bounds
            gimp_drawable_mask_bounds (drawable->drawable_id,
                                       &cx1, &cy1,
                                       &cx2, &cy2);
           
            // Finally crop one last time so the fish is isolated
            result = gimp_image_crop (image_id,
                                      (cx2 - cx1), (cy2 - cy1),
                                      cx1, cy1);
        }
    }
    
    gimp_progress_update (100);
    if (!result && !failed)
    {
        failed = TRUE;
        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            g_message ("Failed to crop out borders");
    }
    
    // Handle failure reporting
    if (failed)
    {
        status = GIMP_PDB_EXECUTION_ERROR;
    }
    
    
    gimp_displays_flush ();
    gimp_drawable_detach (drawable);
    
    // Set return values (we only return 1 values: status)
    *nreturn_vals = 1;
    *return_vals = values;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;
}

// Stores the location of the white fish borders in *c1 and *c2
// If direction == DIR_HORIZONTAL, gets the left and right borders
// If direction == DIR_VERTICAL, gets the top and bottom borders
static void getInitialCrop (GimpDrawable *drawable, gint *c1, gint *c2, enum DIR direction)
{
    // Variables starting with p mean parallel
    // They go along the same direction as the direction param
    // Variables starting with o or orth mean orthogonal
    // They go along the perpendicular direction as the direction param
    // Ex: Direction is DIR_HORIZONTAL, p is the x direction, o (or orth) is the y direction
    // Ex: Direction is DIR_VERTICAL, p is the y direction, o (or orth) is the x direction
    gint    o1, p1, o2, p2, channels;
    gint    i, j, orth_start, orth_end;
    gboolean found_first_border;
    guchar *par1, *orth1;
    GimpPixelRgn  rgn_in;
    
    // Get our boundaries
    // Make sure we set the p and o variables based on direction
    if (direction == DIR_HORIZONTAL)
    {
        gimp_drawable_mask_bounds (drawable->drawable_id,
                                   &p1, &o1,
                                   &p2, &o2);
       
        // We only need to read pixels here
        gimp_pixel_rgn_init (&rgn_in,
                             drawable,
                             p1, o1,
                             p2 - p1, o2 - o1,
                             FALSE, FALSE);
    }
    else if (direction == DIR_VERTICAL)
    {
        gimp_drawable_mask_bounds (drawable->drawable_id,
                                   &o1, &p1,
                                   &o2, &p2);
       
        // We only need to read pixels here
        gimp_pixel_rgn_init (&rgn_in,
                             drawable,
                             o1, p1,
                             o2 - o1, p2 - p1,
                             FALSE, FALSE);
    }
    else
    {
        // If the direction is invalid, stop here
        // Should never happen
        return;
    }
    
    // Figure out the number of bytes per pixel
    channels = gimp_drawable_bpp (drawable->drawable_id);
    
    // Allocate a row/col for one set of pixels in the p and o direction
    par1  = g_new (guchar, channels * (p2 - p1));
    orth1 = g_new (guchar, channels * (o2 - o1));
    
    // Start in the middle of the image to make sure we are inside the borders
    orth_start = o1 + ((o2 - o1) / 2);
    
    // Make sure we get a row (for horizontal) or column (for vertical)
    if (direction == DIR_HORIZONTAL)
    {
        // Get the column we are going to test on
        gimp_pixel_rgn_get_row (&rgn_in,
                                par1,
                                p1, orth_start,
                                p2 - p1);
    }
    else if (direction == DIR_VERTICAL)
    {
        
        // Get the row we are going to test on
        gimp_pixel_rgn_get_col (&rgn_in,
                                par1,
                                orth_start, p1,
                                p2 - p1);
    }
    // Default return values to selection start and selection end (whole image)
    // if we don't find the border (since the user may not have included it in the screenshot)
    *c1 = p1;
    *c2 = p2;
    
    // Determines if we found the first border
    found_first_border = FALSE;
    
    // Loop through the pixels to find the first and second white border
    for (i = p1; i < p2; i++)
    {
        // If we are halfway into the image and haven't
        // found the first border, it probably isn't there
        if (!found_first_border && i > p2 / 2)
        {
           found_first_border = TRUE;
        }
        // Test if the pixel is white enough (like the border)
        if (is_white(par1, channels, i))
        {
            // There will be a lot of white pixels along the perpendicular
            // direction in the border. Check those to make sure this isn't an eye
            if (direction == DIR_HORIZONTAL)
            {
                // Check if the whole column is white (or at least more of it)
                gimp_pixel_rgn_get_col (&rgn_in,
                                        orth1,
                                        i, o1,
                                        o2 - o1);
            }
            else if (direction == DIR_VERTICAL)
            {
                // Check if the whole row is white (or at least more of it)
                gimp_pixel_rgn_get_row (&rgn_in,
                                        orth1,
                                        o1, i,
                                        o2 - o1);
            }

            // Check 20 pixels before and after the first one we checked
            gboolean white = TRUE;
            gint orth_start2 = MAX(orth_start - 20, o1);
            gint orth_end    = MIN(orth_start + 20, o2);
            for (j = orth_start2; j < orth_end; j++)
            {
                // If a pixel isn't white, this sin't the border
                if (!is_white(orth1, channels, j))
                {
                    white = FALSE;
                    break;
                }
            }
            // If every pixel was white, we found the border
            if (white)
            {
                if (!found_first_border)
                {
                    found_first_border = TRUE;
                    // The border is ~2 pixels
                    // Make sure we crop inside it
                    *c1 = MIN(i + 4, p2);
                    i = *c1;
                }
                else
                {
                    // The border is ~2 pixels
                    // Make sure we crop inside it
                    *c2 = MAX(i - 4, p1);
                    break;
                }
            }
        }
    }
    
    // Always free resources
    g_free (par1);
    g_free (orth1);
}

static gboolean is_white (guchar *buf, gint channels, gint offset)
{
    // We only work on RGB or RGBA. Make sure the
    // RGB is very close to white but allow some leeway
    if (buf[channels * offset]     > 225 && // R
        buf[channels * offset + 1] > 225 && // G
        buf[channels * offset + 2] > 225)   // B
        return TRUE;
    return FALSE;
}

