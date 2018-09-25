(define-module (custom tps)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft tick)
               
               #:use-module (megacut))

(reg-simple-command "tps"
                    "Get server tps"
                    0
                  #%(outp-success (format #f "TPS: ~a" (get-tps))))