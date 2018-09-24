(defmacro for-each-player! (player . body)
          `(for-each-player (lambda (,player) ,@body #t)))

(export for-each-player!)