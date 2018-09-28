(define-module (custom vanilla-ext)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft fake)
               #:use-module (minecraft tick)

               #:use-module (megacut)

               #:use-module (ice-9 match)
               
               #:export (init-player-trace))

(define particles
        `("none"
          "bubble"
          "crit"
          "smoke"
          "explode"
          "evaporation"
          "flame"
          "lava"
          "largesmoke"
          "redust"
          "iconcrack"
          "snowballpoof"
          "largeexplode"
          "hugeexplosion"
          "mobflame"
          "heart"
          "terrain"
          "townaura"
          "portal"
          "watersplash"
          "waterwake"
          "dripwater"
          "driplava"
          "fallingdust"
          "mobspell"
          "mobspellambient"
          "mobspellinstantaneous"
          "ink"
          "slime"
          "rainsplash"
          "villagerangry"
          "villagerhappy"
          "enchantingtable"
          "trackingemitter"
          "note"
          "witchspell"
          "carrotboost"
          "dragonbreath"
          "spit"
          "totem"
          "food"
          "conduit"
          "bubblecolumnup"
          "bubblecolumndown"
          
          "forcefield"
          "risingreddust"))

(define particle-enum (apply parameter-enum "particle" "ParticleType" particles))

(reg-command "particle"
             "Add particle"
             1
             (list (command-vtable (list particle-enum)
                                 #%(match (command-args)
                                         [(name) (fake-particle name (orig-pos) (orig-dim)) (outp-success (format #f "~a ~a ~a" name (orig-pos) (orig-dim)))]))
                   (command-vtable (list particle-enum (parameter-position "pos") (parameter-optional parameter-int "data"))
                                 #%(match (command-args)
                                         [(name pos data) (fake-particle name pos (orig-dim) data) (outp-success (format #f "~a ~a ~a ~a" name pos (orig-dim) data))]))))

(define trace-particle "portal")
(define trace-fix 1.6)

(define* (fix-pos pos #:optional (fix 1.6)) (f32vector-set! pos 1 (- (f32vector-ref pos 1) fix)) pos)

(define (init-player-trace name fix) (set! trace-particle name) (set! trace-fix fix))

(interval-run! 1 (for-each-player! player (fake-particle trace-particle (fix-pos (actor-pos player) trace-fix) (actor-dim player) 1)))