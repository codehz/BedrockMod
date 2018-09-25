(define-module (utils form)
               #:use-module (json)

               #:use-module (ice-9 match)
               
               #:export (make-simple-form make-buttons-form))

(define* (make-simple-form title content #:optional (button1 "Accept") (button2 "Reject"))
         (scm->json-string `((title   . ,title)
                             (type    . "modal")
                             (content . ,content)
                             (button1 . ,button1)
                             (button2 . ,button2))))


(define (make-buttons-form title content . buttons)
        (scm->json-string `((title   . ,title)
                            (type    . "form")
                            (content . ,content)
                            (buttons . ,(map (match-lambda [(text 'path path) `((text . ,text) (image . ((type . path) (data . ,path))))]
                                                           [(text 'url url) `((text . ,text) (image . ((type . url) (data . ,path))))]
                                                           [(text . path) `((text . ,text) (image . ((type . path) (data . ,path))))]
                                                           [text `((text . ,text))])
                                       buttons)))))
