;; Copyright 2018 Linus Björnstam <linus.bjornstam@fastmail.se>
;; This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
;; If a copy of the MPL was not distributed with this file, You can obtain one
;; at http://mozilla.org/MPL/2.0/
;;
;;
;;; Clojure-like lambda shorthand for guile.
;; (megacut (let ([a "Hello "]) (string-append a %1)))
;;   => (lambda (%1) (let ([a "Hello"]) (string-append a %1)))
;; It supports rest arguments using the name %&
;; and it also supports ignoring arguments:
;; (megacut (display %3))
;;   => (lambda (%1 %2 %3) (display %3))
;;
;; The shorthand % gets converted to %1, so (megacut (+ % %))
;;   => (lambda (%1) (+ %1 %1))
;;
;; I also provide a clojuresque shorthand:
;; #%(+ % %) => (megacut (+ % %)) => (lambda (%1) (+ %1 %1))
;;
;; Should be trivial to port to any syntax-case scheme


(define-module (megacut)
  #:export (megacut)
  #:use-module (ice-9 regex)
  #:use-module (ice-9 receive)
  #:use-module (srfi srfi-1))


;; This should be made faster. maybe non-tail-recursive?
(define (flatten lst)
  (let loop ([lst lst] [acc '()])
    (cond
     [(null? lst) (reverse acc)]
     [(list? (car lst))
      (loop (cdr lst)
            (append (flatten (car lst)) acc))]
     [else (loop (cdr lst) (cons (car lst) acc))])))

;; match "%[numbers]"
(define rxp (make-regexp "^%([0-9]+)$"))

;; Searches s for %n-styled variables
;; and returns a list like (n n n n)
;; %& means -1, so that we can use max/min on the list without much fuss
;; in get-max
(define (get-thing-or-false s)
  (if (symbol? s)
      (let ([s (symbol->string s)])
        (cond
         ;; Rest arguments are -1, to ba able to use it easily with min and max in get-max
         [(string=? s "%&") -1]
         ;; % means %1, so just return 1
         [(string=? s "%") 1]
         ;; see if we can match rxp and numberify that
         [else
          (let ([match  (regexp-exec rxp s)])
            (and match (string->number (match:substring match 1))))]))
      #f))


;; creates list containing [max] elements of the shape %1 %2 ... %n
;; and also appends ". %&" if we need rest args
(define (make-args-list max rest?)
  (define (make-arg num)
    (string->symbol (string-append "%" (number->string num))))
  (cond
   [(and (<=  max 0) rest?) '%&]
   [rest? `(,@(filter-map make-arg (iota max 1)) . %r)]
   [else (filter-map make-arg (iota max 1))]))

;; get thing or false retuns a list of all the %n-styled args
;; get-max returns a the max of that list, and also a boolean whether we
;; have a rest argument (which we know because lst contains a negative number
;; which it can't otherwise
(define (get-max lst)
  (let ([stuff  (filter-map get-thing-or-false (flatten lst))])
    (values (apply max 0 stuff) (negative? (apply min 0 stuff)))))


(define-syntax megacut
  (lambda (stx)
    (syntax-case stx ()
      [(mcut body ...)
       (receive (max rest?) (get-max (syntax->datum #'(body ...)))
         (with-syntax ([args (datum->syntax stx (make-args-list max rest?))]
                       [% (datum->syntax stx '%)]
                       [%1 (datum->syntax stx '%1)])
           #'(lambda args
               (let-syntax ([% (identifier-syntax %1)])
                 body ...))))])))


(read-hash-extend #\% (lambda (c p) `(megacut ,(read p))))