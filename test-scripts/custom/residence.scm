(define-module (custom residence)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               #:use-module (minecraft command)
               #:use-module (minecraft form)
               #:use-module (minecraft chat)
               #:use-module (minecraft world)
               #:use-module (minecraft fake)
               #:use-module (minecraft tick)

               #:use-module (srfi srfi-26)
               #:use-module (ice-9 match)
               
               #:use-module (utils form)
               #:use-module (utils point-select))

;(reg-simple-command "res-menu"
;                    "Residence MENU"
;                    0
;                    (checked-player! player
;                                     (send-form player
;                                                (make-menu "Residence Menu" "Welcome to use Residence" `("Create" . "textures/blocks/cauldron_bottom.png"))
;                                                (match-lambda [0 (send-message player (format #f "Selected ~a" (select-point player)))])))
;                    (outp-success))