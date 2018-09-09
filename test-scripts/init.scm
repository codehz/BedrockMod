(use-modules (sqlite3))
(use-modules (minecraft))
(use-modules (minecraft base))
(use-modules (minecraft tick))
(use-modules (minecraft dbus))
(use-modules (minecraft chat))
(use-modules (minecraft form))
(use-modules (system repl coop-server))
(use-modules (json))

(log-trace "test"
           "~s - ~a"
           "测试"
           123)

(log-debug "uuid"
           (uuid->string (uuid "107d46e0-4d59-4e51-97ab-6585fe429d94")))

(define (%player-login uuid)
        (log-debug "player-login" (uuid->string uuid))
        #t)

(define (%player-joined player)
        (log-debug "player-joined" "HIT ~a" (actor-name player))
        (for-each-player (λ (other)
                            (send-message other (format #f "~a joined." (actor-name other)))
                            #t)))

(define (handle-custom-command player command)
        (cond ((and player (string=? command "test"))
                 (delay-run! 50 (send-form player
                                           (scm->json-string '((title   . "test")
                                                               (type    . "modal")
                                                               (content . "test")
                                                               (button1 . "ok")
                                                               (button2 . "cancel")))
                                           (λ (res)
                                              (log-debug "result" "form: ~a" (json-string->scm res))))))))

(define (%player-chat player message)
        (if (string-prefix? "." message)
            (handle-custom-command player (string-drop message 1))
            (begin (for-each-player (λ (other)
                                       (send-message other (format #f "~a: ~a" (actor-name other) message))
                                       #t))
                   (log-info "chat" "~a: ~a" (actor-name player) message))))

(define (%server-exec message)
        (if (string-prefix? "/" message)
            #f
            (begin (for-each-player (λ (other)
                                       (send-message other (format #f "server: ~a" message))
                                       #t))
                   (log-info "chat" "server: ~a" message)
                   "")))

(delay-run! 5 (log-debug "delay" "test 5"))
(delay-run! 7 (log-debug "delay" "test 7"))

(register-dbus-interface ""
                         "one.codehz.bedrockserver.test"
                         (λ (vt) #f
                                (define-dbus-signal vt 0 "test_signal" "s")
                                (define-dbus-method vt 0 "test_method" "s" "s"
                                                    (λ (m u e)
                                                       (let ((data (dbus-read m #\s)))
                                                            (dbus-reply m "s" data))))))

(let ((server (spawn-coop-repl-server)))
     (interval-run! 1 (poll-coop-repl-server server)))