(define-module (utils point-select)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               
               #:export (select-point))

(define (select-point player target-item-id callback)
        (letrec [(remove-when-player-left (lambda () (remove-hook! policy-player-use-on fn)))
                 (fn (lambda (item pos vec)
                             (if (actor=? (policy-self) player)
                                 (let [(source-item-id (item-instance-id item))]
                                       (if (eqv? target-item-id source-item-id)
                                           (begin (callback pos)
                                                  (remove-hook! (player-left-for player) remove-when-player-left)
                                                  (remove-hook! policy-player-use-on fn)))))))]
                 (add-hook! policy-player-use-on fn)
                 (add-hook! (player-left-for player) remove-when-player-left)))