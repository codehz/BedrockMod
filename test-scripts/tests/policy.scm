(define-module (tests policy)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               #:use-module (minecraft world)
               #:use-module (minecraft chat)
               #:use-module (minecraft command)
               
               #:use-module (megacut)
               
               #:use-module (ice-9 match))

(define (actor-dump actor)
        (string-concatenate/shared (map (lambda (x) (string-append x ";")) (actor-debug-info actor))))

(define (show-vec3 vec)
        (match (f32vector->list vec)
              [(x y z) (format #f "(~1,2f ~1,2f ~1,2f)" x y z)]))

(add-hook! policy-player-attack
         #%(send-message (policy-self) (format #f "Attack ~a" (actor-dump %))))

(add-hook! policy-player-destroy
         #%(send-message (policy-self) (format #f "Destroy@~a ~a" % (block-name (get-block/player % (policy-self))))))

(reg-command "dump-block"
             "Dump block for position"
             0
             (list (command-vtable (list (parameter-position "pos") (parameter-int "dim"))
                                   (lambda ()
                                           (match (command-args)
                                                 [(pos dim) (outp-success (block-name (get-block (vec3->blockpos pos) dim)))])))))

(add-hook! policy-player-interact
         #%(send-message (policy-self) (format #f "Interact@~a ~a" (show-vec3 %2) (actor-dump %))))

(add-hook! policy-player-use
         #%(send-message (policy-self) (format #f "Use ~a" (item-instance-debug-info %))))

(add-hook! policy-player-use-on
         #%(send-message (policy-self) (format #f "Use ~a ~a ~a" (item-instance-debug-info %) %2 (show-vec3 %3))))
