(define-module (custom ping)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft chat)
               #:use-module (minecraft tick)
               
               #:export (init-stats-bar))

(reg-simple-command "ping"
                    "Get network stats"
                    0
                    (checked-player! player (outp-success (apply format #f "ping: ~ams/~ams loss: ~1,2f/~1,2f" (player-stats player)))))

(define (init-stats-bar action)
        (interval-run! 20 (for-each-player! player (action player))))