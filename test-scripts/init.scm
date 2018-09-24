(use-modules (minecraft))

(log-debug "guile" "loading")

(use-modules (tests command))
(use-modules (tests misc))
(use-modules (tests nbt))
(use-modules (tests policy))

(use-modules (custom block-items))
(use-modules (custom lucky-block))
(use-modules (custom ping))
(use-modules (custom player-events))
(use-modules (custom teleport))
(use-modules (custom transfer))

(log-debug "guile" "done")