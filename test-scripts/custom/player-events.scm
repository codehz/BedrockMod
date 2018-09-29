(define-module (custom player-events)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft chat)
               #:use-module (minecraft server-exec)
               
               #:use-module (megacut))

(add-hook! player-joined
           #%(let [(pname (actor-name %))]
                   (log-debug "player-joined" "~a ~a ~a" pname (uuid->string (player-uuid %)) (player-xuid %))
                   (for-each-player! other (send-message other (format #f "~a joined." pname)))))

(add-hook! player-left
           #%(let [(pname (actor-name %))]
                   (log-debug "player-left" "~a ~a ~a" pname (uuid->string (player-uuid %)) (player-xuid %))
                   (for-each-player! other (send-message other (format #f "~a left." pname)))))

(add-hook! player-login
           #%(log-debug "player-login" (uuid->string %)))

(add-hook! player-chat
           #%(let [(pname (actor-name %1))]
                   (for-each-player! other (send-message other (format #f "~a: ~a" pname %2)))
                   (log-info "chat" "~a: ~a" pname %2)
                   (cancel-chat #t)))

(add-hook! server-exec
           #%(cond ((string-prefix? "/" %) (exec-result #f))
                   (else (for-each-player! player (send-message player (format #f "server: ~a" %)))
                         (log-info "chat" "server: ~a" %)
                         (exec-result ""))))

