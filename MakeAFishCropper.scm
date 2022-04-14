;; MakeAFishCropper.scm: Batch run the MakeAFishCropper plugin

  ;; Take a filename pattern as a parameter
  (define (batch-makeafishcropper pattern)
  (let* ((filelist (cadr (file-glob pattern 1))))
    (while (not (null? filelist))
           (let* ((filename (car filelist))
                  (image (car (gimp-file-load RUN-NONINTERACTIVE
                                              filename filename)))
                  (drawable (car (gimp-image-get-active-layer image))))
             (plug-in-makeafishcropper RUN-NONINTERACTIVE
                                   image drawable)
             (gimp-file-save RUN-NONINTERACTIVE
                             image drawable filename filename)
             (gimp-image-delete image))
           (set! filelist (cdr filelist)))))
