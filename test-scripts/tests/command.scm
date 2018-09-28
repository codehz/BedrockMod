(define-module (tests command)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               
               #:use-module (megacut))

(reg-simple-command "script"
                    "Custom command from script"
                    0
                    #%(outp-success "Hello guile!"))

(reg-command "message"
             "Custom command for testing message"
             0
             (list (command-vtable (list (parameter-optional parameter-message "test")) #%(outp-success (format #f "You typed ~a" (command-args))))))

(reg-command "select"
             "Custom command for testing selector"
             0
             (list (command-vtable (list (parameter-optional parameter-selector "sth")) #%(outp-success (format #f "Selected ~a" (command-args))))))

(reg-command "string"
             "Custom command for testing string"
             0
             (list (command-vtable (list (parameter-optional parameter-string "test")) #%(outp-success (format #f "String ~a" (command-args))))))

(reg-command "text"
             "Custom command for testing text"
             0
             (list (command-vtable (list (parameter-optional parameter-text "test")) #%(outp-success (format #f "Text ~a" (command-args))))))

(reg-command "int"
             "Custom command for testing int"
             0
             (list (command-vtable (list (parameter-optional parameter-int "test")) #%(outp-success (format #f "Integer ~a" (command-args))))))

(reg-command "float"
             "Custom command for testing float"
             0
             (list (command-vtable (list (parameter-optional parameter-float "test")) #%(outp-success (format #f "Float ~a" (command-args))))))

(reg-command "bool"
             "Custom command for testing boolean"
             0
             (list (command-vtable (list (parameter-optional parameter-bool "test")) #%(outp-success (format #f "Boolean ~a" (command-args))))))

(reg-command "position"
             "Custom command for testing position"
             0
             (list (command-vtable (list (parameter-optional parameter-position "test")) #%(outp-success (format #f "Pos ~a" (command-args))))))

(reg-command "enum"
             "Custom command for testing enum"
             0
             (list (command-vtable (list (parameter-enum "test" "TENUM" "X" "Y" "Z"))
                                 #%(outp-success (format #f "Enum ~a" (command-args))))))

(reg-command "multiple"
             "Custom command for testing multiple parameters"
             0
             (list (command-vtable (list (parameter-position "pos") (parameter-selector "sel") (parameter-text "text"))
                                 #%(outp-success (format #f "Multiple parameters ~a" (command-args))))
                   (command-vtable (list (parameter-selector "sel") (parameter-text "text"))
                                 #%(outp-success (format #f "Multiple parameters ~a" (command-args))))))
