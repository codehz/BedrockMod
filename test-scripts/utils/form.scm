(define-module (utils form)
               #:use-module (json)

               #:use-module (ice-9 match)
               
               #:export (make-simple-form
                         make-menu
                         make-custom-form
                         make-settings-form))

(define* (make-simple-form title content #:optional (button1 "Accept") (button2 "Reject"))
         (scm->json-string `((title   . ,title)
                             (type    . "modal")
                             (content . ,content)
                             (button1 . ,button1)
                             (button2 . ,button2))))

(define transform-icon
        (match-lambda [('path path) `((type . path) (data . ,path))]
                      [('url url) `((type . url) (data . ,url))]
                      [path `((type . path) (data . ,path))]))

(define transform-button
        (match-lambda [(text . icon) `((text . ,text) (image . ,(transform-icon icon)))]
                      [text `((text . ,text))]))

(define (make-menu title content . buttons)
        (scm->json-string `((title   . ,title)
                            (type    . "form")
                            (content . ,content)
                            (buttons . ,(map transform-button buttons)))))

(define* (make-custom-form title . content)
         (scm->json-string  `((title . ,title)
                              (type  . "custom_form")
                              (content . ,content))))

(define* (make-settings-form title icon . content)
         (scm->json-string  `((title . ,title)
                              (type  . "custom_form")
                              (icon . ,(transform-icon icon))
                              (content . ,content))))