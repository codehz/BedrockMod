(define-module (custom ping)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft chat)
               #:use-module (minecraft tick))

(interval-run! 20 (for-each-player! player (send-message player (apply format #f "ping: ~ams(avg ~ams) loss: ~1,2f(avg ~1,2f)" (player-stats player)) 5)))

(reg-simple-command "ping"
                    "Get network stats"
                    0
                    (checked-player! player (outp-success (format #f "~a" (player-stats player)))))
