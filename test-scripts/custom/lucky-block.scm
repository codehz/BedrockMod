(define-module (custom lucky-block)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               #:use-module (minecraft world)
               #:use-module (minecraft fake)
               #:use-module (minecraft tick)
               
               #:export (init-lucky-block))

(define (init-lucky-block target lucky-actions)
        (add-hook! policy-player-destroy
                   (lambda (pos)
                           (let [(blockname (block-name (get-block/player pos (policy-self))))]
                                 (if (string=? target blockname)
                                     (let [(player (policy-self))]
                                           (clear-block/player pos player)
                                           (policy-result #f)
                                           (fake-explode 10.0 (actor-pos player) (actor-dim player))
                                           (delay-run! 1 ((list-ref lucky-actions (random (length lucky-actions))) player pos))))))))