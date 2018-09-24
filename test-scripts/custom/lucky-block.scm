(define-module (custom lucky-block)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               #:use-module (minecraft world)
               #:use-module (minecraft fake)
               #:use-module (minecraft tick))

(define lucky-actions
        (letrec [(ret '())
                 (append-action (lambda (n action)
                                        (for-each (lambda (x) (set! ret (cons action ret))) (iota n))))]
                 ; (append-action 1 (lambda (player pos) (player-spawn-actor (blockpos->vec3 pos) player "minecraft:tnt")))
                 (append-action 3 (lambda (player pos) (player-spawn-actor (blockpos->vec3 pos) player "minecraft:villager")))
                 (append-action 3 (lambda (player pos) (player-set-block@ pos player "minecraft:diamond_block")))
                 (append-action 5 (lambda (player pos) (player-set-block@ pos player "minecraft:stone")))
                 ret))

(add-hook! policy-player-destroy
           (lambda (pos)
                   (let [(blockname (block-name (player-get-block@ pos (policy-self))))]
                         (if (string=? "minecraft:element_0" blockname)
                             (let [(player (policy-self))]
                                   (player-clear-block@ pos player)
                                   (policy-result #f)
                                   (fake-explode 10.0 (actor-pos player) (actor-dim player))
                                   (delay-run! 1 ((list-ref lucky-actions (random (length lucky-actions))) player pos)))))))
