(use-modules (minecraft)
             (minecraft base)
             (minecraft chat)
             (minecraft world))

(log-debug "guile" "loading")

(use-modules (tests command))
(use-modules (tests misc))
(use-modules (tests nbt))
(use-modules (tests policy))
(use-modules (tests database))

(use-modules (custom prevent-action))
(use-modules (custom lucky-block))
(use-modules (custom ping))
(use-modules (custom tps))
(use-modules (custom player-events))
(use-modules (custom teleport))
(use-modules (custom transfer))

(log-debug "guile" "config")

(init-lucky-block "minecraft:element_0"
                  (letrec [(ret '())
                           (append-action (lambda (n action)
                                                  (for-each (lambda (x) (set! ret (cons action ret))) (iota n))))]
                           ; (append-action 1 (lambda (player pos) (player-spawn-actor (blockpos->vec3 pos) player "minecraft:tnt")))
                           (append-action 3 (lambda (player pos) (player-spawn-actor (blockpos->vec3 pos) player "minecraft:villager")))
                           (append-action 3 (lambda (player pos) (player-set-block@ pos player "minecraft:diamond_block")))
                           (append-action 5 (lambda (player pos) (player-set-block@ pos player "minecraft:stone")))
                           ret))

(init-stats-bar (lambda (player) (send-message player (apply format #f "ping: ~ams/~ams loss: ~1,2f/~1,2f" (player-stats player)) 5)))

(set-teleport-cooldown! 500)

(prevent-item-use (map lookup-item-id '("minecraft:bedrock" "minecraft:barrier")))
(prevent-block-destroy '("minecraft:bedrock"))

(log-debug "guile" "done")