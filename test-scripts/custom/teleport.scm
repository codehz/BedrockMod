(define-module (custom teleport)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft command)
               #:use-module (minecraft form)
               #:use-module (minecraft tick)
               #:use-module (minecraft chat)

               #:use-module (utils form)
               #:use-module (megacut)
               
               #:use-module (srfi srfi-1)
               #:use-module (ice-9 match))

(define* (fix-pos pos #:optional (fix 1.6)) (f32vector-set! pos 1 (- (f32vector-ref pos 1) fix)))

(reg-simple-command "home"
                    "Teleport to spawnpoint"
                    0
                    (checked-player! player
                                     (let* [(pos (player-spawnpoint player))
                                            (point (blockpos->vec3 pos))]
                                            (fix-pos point 1)
                                            (teleport player point 0)
                                            (outp-success))))

(reg-simple-command "spawn"
                    "Teleport to world spawnpoint"
                    0
                    (checked-player! player
                                     (let* [(pos (world-spawnpoint))
                                            (point (blockpos->vec3 pos))]
                                            (fix-pos point 1)
                                            (teleport player point 0)
                                            (outp-success))))

(add-hook! player-joined
         #%(delay-run! 100 (send-form %
                                      (make-simple-form "Hello" "world" "Done" "Dismiss")
                                      (lambda (x) x))))

(define tp-cooldown-list '())
(define tp-cooldown 1000)

(define (tp from to)
        (let [(pos (actor-pos to))
              (dim (actor-dim to))]
              (fix-pos pos)
              (teleport from pos dim)))

(reg-command "tpa"
             "Send a teleport request to other player"
             0
             (list (command-vtable (list (parameter-selector "target" #t))
                                   (checked-player! self
                                                    (match (cons (lset<= uuid=? (list (player-uuid self)) tp-cooldown-list) (command-args))
                                                          [(#t _) (outp-error "Waiting for teleport cooldown")]
                                                          [(#f (target)) (send-form target
                                                                                    (make-simple-form "Teleport request" (format #f "From ~a" (actor-name self)))
                                                                                  #%(if (json-string->scm %)
                                                                                        (begin (tp self target)
                                                                                               (send-message self "Teleported."))
                                                                                        (send-message self "Request rejected.")))
                                                                         (let [(uuid (player-uuid self))]
                                                                               (set! tp-cooldown-list (lset-adjoin uuid=? tp-cooldown-list uuid))
                                                                               (delay-run! tp-cooldown (set! tp-cooldown-list (lset-difference uuid=? tp-cooldown-list (list uuid)))))
                                                                         (outp-success "Request sent.")]
                                                          [_ (outp-error "Must have 1 player selected")])))))

(reg-command "tpahere"
             "Send a teleport here request to other player"
             0
             (list (command-vtable (list (parameter-selector "target" #t))
                                   (checked-player! self
                                                    (match (cons (lset<= uuid=? (list (player-uuid self)) tp-cooldown-list) (command-args))
                                                          [(#t _) (outp-error "Waiting for teleport cooldown")]
                                                          [(#f (target)) (send-form target
                                                                                    (make-simple-form "Teleport request" (format #f "To ~a" (actor-name self)))
                                                                                  #%(if (json-string->scm %)
                                                                                        (begin (tp target self)
                                                                                               (send-message self "Teleported."))
                                                                                        (send-message self "Request rejected.")))
                                                                         (let [(uuid (player-uuid self))]
                                                                               (set! tp-cooldown-list (lset-adjoin uuid=? tp-cooldown-list uuid))
                                                                               (delay-run! tp-cooldown (set! tp-cooldown-list (lset-difference uuid=? tp-cooldown-list (list uuid)))))
                                                                         (outp-success "Request sent.")]
                                                          [_ (outp-error "Must have 1 player selected")])))))
