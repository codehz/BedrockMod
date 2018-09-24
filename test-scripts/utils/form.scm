(define-module (utils form)
               #:use-module (json)
               
               #:export (make-simple-form))

(define* (make-simple-form title content #:optional (button1 "Accept") (button2 "Reject"))
         (scm->json-string `((title   . ,title)
                             (type    . "modal")
                             (content . ,content)
                             (button1 . ,button1)
                             (button2 . ,button2))))
