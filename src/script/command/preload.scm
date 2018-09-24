
(defmacro checked-player! (name . body)
`(lambda () (let [(,name (orig-player))]
                  (if (not ,name)
                      (outp-error "Only available for player")
                      (begin ,@body)))))

(export checked-player!)