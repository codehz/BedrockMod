(use-modules (minecraft))
(use-modules (minecraft base))
(use-modules (minecraft tick))
(use-modules (minecraft dbus))
(use-modules (minecraft chat))
(use-modules (minecraft form))
(use-modules (minecraft command))
(use-modules (system repl coop-server))
(use-modules (json))
(use-modules (megacut))
(use-modules (sqlite3))

(log-trace "test"
           "~s - ~a"
           "测试"
           123)

(log-debug "uuid"
           (uuid->string (uuid "107d46e0-4d59-4e51-97ab-6585fe429d94")))

(add-hook! player-joined
           #%(let [(pname (actor-name %))]
                   (log-debug "player-joined" "HIT ~a ~a ~a" pname (uuid->string (player-uuid %)) (player-xuid %))
                   (for-each-player! other (send-message other (format #f "~a joined." pname)))))

(add-hook! player-login
           #%(log-debug "player-login" (uuid->string %)))

(add-hook! player-chat
           #%(let [(pname (actor-name %1))]
                   (for-each-player! other (send-message other (format #f "~a: ~a" pname %2)))
                   (log-info "chat" "~a: ~a" pname %2)))

(add-hook! server-exec
           #%(cond ((string-prefix? "/" %) (exec-result #f))
                   (else (for-each-player! player (send-message player (format #f "server: ~a" %)))
                         (log-info "chat" "server: ~a" %)
                         (exec-result ""))))

(delay-run! 5 (log-debug "delay" "test 5"))
(delay-run! 7 (log-debug "delay" "test 7"))

(interval-run! 20 (for-each-player! player (send-message player (apply format #f "ping: ~ams(avg ~ams) loss: ~a(avg ~a)" (player-stats player)) 5)))

(register-dbus-interface ""
                         "one.codehz.bedrockserver.test"
                         (megacut (define-dbus-signal % 0 "test_signal" "s")
                                  (define-dbus-method % 0 "test_method" "s" "s"
                                                      (λ (m u e)
                                                         (let [(data (dbus-read m #\s))]
                                                              (dbus-reply m "s" data))))))

(reg-simple-command "script"
                    "Custom command from script"
                    0
                    #%(outp-success %3 "Hello guile!"))

(reg-command "script2"
             "Custom command from script2"
             0
             (list (command-vtable (list (parameter-message "test")) #%(outp-success %3 (format #f "You typed ~a" (command-args %1 %2))))))

(reg-command "select"
             "Custom command for testing selector"
             0
             (list (command-vtable (list (parameter-selector "sth")) #%(outp-success %3 (format #f "Selected ~a" (command-args %1 %2))))))

(reg-command "string"
             "Custom command for testing string"
             0
             (list (command-vtable (list (parameter-string "test")) #%(outp-success %3 (format #f "String ~a" (command-args %1 %2))))))

(reg-command "text"
             "Custom command for testing text"
             0
             (list (command-vtable (list (parameter-text "test")) #%(outp-success %3 (format #f "Text ~a" (command-args %1 %2))))))

(reg-command "int"
             "Custom command for testing int"
             0
             (list (command-vtable (list (parameter-int "test")) #%(outp-success %3 (format #f "Integer ~a" (command-args %1 %2))))))

(reg-command "float"
             "Custom command for testing float"
             0
             (list (command-vtable (list (parameter-float "test")) #%(outp-success %3 (format #f "Float ~a" (command-args %1 %2))))))


(reg-command "position"
             "Custom command for testing position"
             0
             (list (command-vtable (list (parameter-position "test")) #%(outp-success %3 (format #f "Pos ~a" (command-args %1 %2))))))

(reg-simple-command "test"
                    "Test form"
                    0
                    #%(let [(player (orig-player %2))]
                            (if (not player)
                                (outp-error %3 "Only available for player")
                                (begin (send-form player
                                                  (scm->json-string '((title   . "test")
                                                                      (type    . "modal")
                                                                      (content . "test")
                                                                      (button1 . "ok")
                                                                      (button2 . "cancel")))
                                                  #%(log-debug "result" "form: ~a" (json-string->scm %)))
                                       (outp-success %3)))))

(reg-simple-command "ping"
                    "Get network stats"
                    0
                    #%(let [(player (orig-player %2))]
                            (if (not player)
                                (outp-error %3 "Only available for player")
                                (outp-success %3 (format #f "~a" (player-stats player))))))

(let [(server (spawn-coop-repl-server))]
      (interval-run! 1 (poll-coop-repl-server server)))