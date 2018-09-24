(define-module (tests nbt)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft nbt)

               #:use-module (megacut)
               
               #:use-module (ice-9 match)
               #:use-module (ice-9 pretty-print))

(reg-command "dump-nbt"
             "Dump NBT"
             0
             (list (command-vtable '()
                                    (checked-player! self
                                                     (with-nbt (actor-nbt self)
                                                               (lambda (nbt)
                                                                       (outp-success (call-with-output-string #%(pretty-print (nbt-unbox-rec/tag nbt) % #:width 200)))))))
                   (command-vtable (list (parameter-selector "target"))
                                 #%(match (command-args)
                                         [(()) (outp-error "No actors selected")]
                                         [(actors) (for-each (lambda (actor)
                                                                     (with-nbt (actor-nbt actor)
                                                                               (lambda (nbt)
                                                                                       (outp-add (call-with-output-string (lambda (port)
                                                                                                                                  (pretty-print (nbt-unbox-rec/tag nbt)
                                                                                                                                                port
                                                                                                                                                #:width 200)))))))
                                                              actors)
                                                    (outp-success)]))))
