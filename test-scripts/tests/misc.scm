(define-module (tests misc)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft dbus)
               #:use-module (minecraft form)
               #:use-module (minecraft chat)

               #:use-module (utils form)
               #:use-module (utils point-select)
               #:use-module (megacut)
               #:use-module (json)

               #:use-module (ice-9 match))

(reg-command "setplayerspawn"
             "Set player's spawnpoint"
             1
             (list (command-vtable (list (parameter-selector "target" #t) (parameter-position "pos"))
                                 #%(match (command-args)
                                         [(() _) (outp-error "No players selected")]
                                         [(players vec) (let [(pos (vec3->blockpos vec))]
                                                              (for-each (lambda (player) (set-player-spawnpoint player pos)) players)
                                                              (outp-success))]))))

(reg-command "change-dim"
             "Change player's dimension"
             1
             (list (command-vtable (list (parameter-int "dim"))
                                   (checked-player! player
                                                    (match (command-args)
                                                          [(dim) (actor-dim-set! player dim)])))))

(reg-simple-command "test-suspend"
                    "Test suspend"
                    0
                    (checked-player! player
                                     (player-suspend player)))

(reg-simple-command "test-resume"
                    "Test resume"
                    0
                    (checked-player! player
                                     (player-resume player)))

(reg-simple-command "test-form"
                    "Test form"
                    0
                    (checked-player! player
                                     (send-form player
                                                (make-simple-form "Test" "test form" "Ok" "Dismiss")
                                              #%(send-message player %))
                                     (outp-success)))

(reg-simple-command "test-form2"
                    "Test Type2 form"
                    0
                    (checked-player! player
                                     (send-form player
                                                (make-menu "Test2" "Type2 content" `("Command Block" . "textures/blocks/command_block.png") "Button2")
                                              #%(send-message player %))
                                     (outp-success)))

(reg-simple-command "test-form3"
                    "Test Type3 form"
                    0
                    (checked-player! player
                                     (send-form player
                                                (make-custom-form "Custom form testing"
                                                                 `((type . "label") (text . "Test Label"))
                                                                 `((type . "input") (text . "Input") (placeholder . "placeholder")))
                                              #%(send-message player %))
                                     (outp-success)))

(reg-simple-command "test-inventory"
                    "Test open inventory"
                    0
                    (checked-player! player
                                     (player-open-inventory player)
                                     (outp-success)))

(reg-simple-command "select-point"
                    "Test point select"
                    0
                    (checked-player! player
                                     (select-point player
                                                   (lookup-item-id "minecraft:stick")
                                                   (lambda (pos)
                                                           (send-message player (format #f "You just selected ~a" pos))))
                                     (outp-success)))

(register-dbus-interface ""
                         "one.codehz.bedrockserver.test"
                         (megacut (define-dbus-signal % 0 "test_signal" "s")
                                  (define-dbus-method % 0 "test_method" "s" "s"
                                                      (λ (m u e)
                                                         (let [(data (dbus-read m #\s))]
                                                              (dbus-reply m "s" data))))))

(add-hook! open-server-settings
           (lambda (player) (send-settings-form player
                                                (make-settings-form "Custom Settings :P"
                                                                    "textures/blocks/command_block.png"
                                                                   `((type . "label") (text . "Test Label"))
                                                                   `((type . "input") (text . "Input") (placeholder . "placeholder")))
                                              #%(log-debug "ServerSettings" "~a" %))))