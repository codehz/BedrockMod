(define-module (custom transfer)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft transfer)

               #:use-module (megacut)
               
               #:use-module (ice-9 match))

(reg-command "transfer"
             "Transfer player to another server"
             1
             (list (command-vtable (list (parameter-selector "target" #t) (parameter-string "address") (parameter-int "port"))
                                 #%(match (command-args)
                                         [((player) address port) (player-transfer player address port) (outp-success)]
                                         [_                       (outp-error "Must have 1 player selected")]))))
