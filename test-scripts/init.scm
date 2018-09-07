(use-modules (language ecmascript parse))
(use-modules ((system base compile) #:select (compile)))
(use-modules (system foreign))
(use-modules (oop goops))
(use-modules (sqlite3))
(use-modules (minecraft))
(use-modules (minecraft base))
(use-modules (minecraft tick))
(use-modules (minecraft dbus))
(use-modules (minecraft chat))
(use-modules (system repl coop-server))

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
                            (send-message other (simple-format #f "~a joined." (actor-name other)))
                            #t)))

(define (%player-chat player message)
        (for-each-player (λ (other)
                            (send-message other (simple-format #f "~a: ~a" (actor-name other) message))
                            #t))
        (log-info "chat" "~a: ~a" (actor-name player) message))

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