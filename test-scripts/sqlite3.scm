;; Guile-SQLite3
;; Copyright (C) 2010, 2014 Andy Wingo <wingo at pobox dot com>
;; Copyright (C) 2018 Ludovic Courtès <ludo@gnu.org>

;; This library is free software; you can redistribute it and/or modify
;; it under the terms of the GNU Lesser General Public License as
;; published by the Free Software Foundation; either version 3 of the
;; License, or (at your option) any later version.
;;                                                                  
;; This library is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; Lesser General Public License for more details.
;;                                                                  
;; You should have received a copy of the GNU Lesser General Public
;; License along with this program; if not, contact:
;;
;; Free Software Foundation           Voice:  +1-617-542-5942
;; 59 Temple Place - Suite 330        Fax:    +1-617-542-2652
;; Boston, MA  02111-1307,  USA       gnu@gnu.org

;;; Commentary:
;;
;; A Guile binding for sqlite.
;;
;;; Code:

(define-module (sqlite3)
  #:use-module (system foreign)
  #:use-module (rnrs bytevectors)
  #:use-module (ice-9 match)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-9)
  #:use-module (srfi srfi-19)
  #:export (sqlite-open
            sqlite-db?
            sqlite-close

	    sqlite-enable-load-extension
            sqlite-exec
            sqlite-prepare
            sqlite-bind
            sqlite-bind-arguments
            sqlite-column-names
            sqlite-step
            sqlite-fold
            sqlite-fold-right
            sqlite-map
            sqlite-reset
            sqlite-finalize
            sqlite-bind-parameter-index
            sqlite-busy-timeout

            SQLITE_OPEN_READONLY
            SQLITE_OPEN_READWRITE
            SQLITE_OPEN_CREATE
            SQLITE_OPEN_DELETEONCLOSE
            SQLITE_OPEN_EXCLUSIVE
            SQLITE_OPEN_MAIN_DB
            SQLITE_OPEN_TEMP_DB
            SQLITE_OPEN_TRANSIENT_DB
            SQLITE_OPEN_MAIN_JOURNAL
            SQLITE_OPEN_TEMP_JOURNAL
            SQLITE_OPEN_SUBJOURNAL
            SQLITE_OPEN_MASTER_JOURNAL
            SQLITE_OPEN_NOMUTEX
            SQLITE_OPEN_FULLMUTEX
            SQLITE_OPEN_SHAREDCACHE
            SQLITE_OPEN_PRIVATECACHE

            SQLITE_CONSTRAINT
            SQLITE_CONSTRAINT_PRIMARYKEY
            SQLITE_CONSTRAINT_UNIQUE))

;;
;; Utils
;;
(define (string->utf8-pointer s)
  (string->pointer s "utf-8"))

(define (utf8-pointer->string p)
  (pointer->string p -1 "utf-8"))


;;
;; Constants
;;

;; FIXME: snarf using compiler. These are just copied from the header...
;;
(define SQLITE_OPEN_READONLY         #x00000001) ;; Ok for sqlite3_open_v2()
(define SQLITE_OPEN_READWRITE        #x00000002) ;; Ok for sqlite3_open_v2()
(define SQLITE_OPEN_CREATE           #x00000004) ;; Ok for sqlite3_open_v2()
(define SQLITE_OPEN_DELETEONCLOSE    #x00000008) ;; VFS only
(define SQLITE_OPEN_EXCLUSIVE        #x00000010) ;; VFS only
(define SQLITE_OPEN_MAIN_DB          #x00000100) ;; VFS only
(define SQLITE_OPEN_TEMP_DB          #x00000200) ;; VFS only
(define SQLITE_OPEN_TRANSIENT_DB     #x00000400) ;; VFS only
(define SQLITE_OPEN_MAIN_JOURNAL     #x00000800) ;; VFS only
(define SQLITE_OPEN_TEMP_JOURNAL     #x00001000) ;; VFS only
(define SQLITE_OPEN_SUBJOURNAL       #x00002000) ;; VFS only
(define SQLITE_OPEN_MASTER_JOURNAL   #x00004000) ;; VFS only
(define SQLITE_OPEN_NOMUTEX          #x00008000) ;; Ok for sqlite3_open_v2()
(define SQLITE_OPEN_FULLMUTEX        #x00010000) ;; Ok for sqlite3_open_v2()
(define SQLITE_OPEN_SHAREDCACHE      #x00020000) ;; Ok for sqlite3_open_v2()
(define SQLITE_OPEN_PRIVATECACHE     #x00040000) ;; Ok for sqlite3_open_v2()

(define SQLITE_CONSTRAINT 19)
(define SQLITE_CONSTRAINT_PRIMARYKEY
  (logior SQLITE_CONSTRAINT (ash 6 8)))
(define SQLITE_CONSTRAINT_UNIQUE
  (logior SQLITE_CONSTRAINT (ash 8 8)))

(define libsqlite3 (dynamic-link "libsqlite3"))

(define-record-type <sqlite-db>
  (make-db pointer open? stmts)
  db?
  (pointer db-pointer)
  (open? db-open? set-db-open?!)
  (stmts db-stmts))

(define-syntax sqlite-db?
  (identifier-syntax db?))

(define-record-type <sqlite-stmt>
  (make-stmt pointer live? reset? cached?)
  stmt?
  (pointer stmt-pointer)
  (live? stmt-live? set-stmt-live?!)
  (reset? stmt-reset? set-stmt-reset?!)
  (cached? stmt-cached? set-stmt-cached?!))

(define sqlite-errmsg
  (let ((f (pointer->procedure
            '*
            (dynamic-func "sqlite3_errmsg" libsqlite3)
            (list '*))))
    (lambda (db)
      (utf8-pointer->string (f (db-pointer db))))))

(define sqlite-errcode
  (let ((f (pointer->procedure
            int
            (dynamic-func "sqlite3_extended_errcode" libsqlite3)
            (list '*))))
    (lambda (db)
      (f (db-pointer db)))))

(define* (sqlite-error db who #:optional code
                       (errmsg (and db (sqlite-errmsg db))))
  (throw 'sqlite-error who code errmsg))

(define* (check-error db #:optional who)
  (let ((code (sqlite-errcode db)))
    (if (not (zero? code))
        (sqlite-error db who code))))

(define sqlite-close
  (let ((f (pointer->procedure
            int
            (dynamic-func "sqlite3_close" libsqlite3)
            (list '*))))
    (lambda (db)
      (when (db-open? db)
        ;; Finalize cached statements.
        (hash-for-each (lambda (sql stmt)
                         (set-stmt-cached?! stmt #f)
                         (sqlite-finalize stmt))
                       (db-stmts db))
        (hash-clear! (db-stmts db))

        (let ((p (db-pointer db)))
          (set-db-open?! db #f)
          (f p))))))

(define db-guardian (make-guardian))
(define (pump-db-guardian)
  (let ((db (db-guardian)))
    (if db
        (begin
          (sqlite-close db)
          (pump-db-guardian)))))
(add-hook! after-gc-hook pump-db-guardian)

(define (static-errcode->errmsg code)
  (case code
    ((1) "SQL error or missing database")
    ((2) "Internal logic error in SQLite")
    ((3) "Access permission denied")
    ((5) "The database file is locked")
    ((6) "A table in the database is locked")
    ((7) "A malloc() failed")
    ((8) "Attempt to write a readonly database")
    ((10) "Some kind of disk I/O error occurred")
    ((11) "The database disk image is malformed")
    ((14) "Unable to open the database file")
    ((21) "Library used incorrectly")
    ((22) "Uses OS features not supported on host")
    ((23) "Authorization denied")
    ((24) "Auxiliary database format error")
    ((26) "File opened that is not a database file")
    (else "Unknown error")))

(define sqlite-open
  (let ((f (pointer->procedure
            int
            (dynamic-func "sqlite3_open_v2" libsqlite3)
            (list '* '* int '*))))
    (lambda* (filename #:optional
                       (flags (logior SQLITE_OPEN_READWRITE SQLITE_OPEN_CREATE))
                       (vfs #f))
      (let* ((out-db (bytevector->pointer (make-bytevector (sizeof '*) 0)))
             (ret (f (string->utf8-pointer filename)
                     out-db
                     flags
                     (if vfs (string->utf8-pointer vfs) %null-pointer))))
        (if (zero? ret)
            (let ((db (make-db (dereference-pointer out-db) #t
                               (make-hash-table))))
              (db-guardian db)
              db)
            (sqlite-error #f 'sqlite-open ret (static-errcode->errmsg ret)))))))

(define sqlite-enable-load-extension
  (let ((ele (pointer->procedure
	      int
	      (dynamic-func "sqlite3_enable_load_extension" libsqlite3)
	      (list '* int))))
    (lambda (db onoff)
      (ele (db-pointer db) onoff))))

(define sqlite-exec
  (let ((exec (pointer->procedure
               int
               (dynamic-func "sqlite3_exec" (@@ (sqlite3) libsqlite3))
               '(* * * * *))))
    (lambda* (db sql)
      "Evaluate the string SQL, which may contain one or several SQL
statements, into DB.  The result is unspecified."
      ;; XXX: 'sqlite3_exec' has a 'callback' parameter but we ignore it
      ;; here.
      (assert-live-db! db)
      (unless (zero? (exec (db-pointer db) (string->pointer sql)
                           %null-pointer %null-pointer %null-pointer))
        (check-error db 'sqlite-exec)))))


;;;
;;; SQL statements
;;;

(define sqlite-remove-statement!
  (lambda (db stmt)
    (when (stmt-cached? stmt)
      (let* ((stmts (db-stmts db))
             (key   (catch 'value
                      (lambda ()
                        (hash-for-each (lambda (key value)
                                         (when (eq? value stmt)
                                           (throw 'value key)))
                                       stmts)
                        #f)
                      (lambda (_ key) key))))
        (hash-remove! stmts key)))))

(define sqlite-finalize
  (let ((f (pointer->procedure
            int
            (dynamic-func "sqlite3_finalize" libsqlite3)
            (list '*))))
    (lambda (stmt)
      ;; Note: When STMT is cached, this is a no-op.  This ensures caching
      ;; actually works while still separating concerns: users can turn
      ;; caching on and off without having to change the rest of their code.
      (when (and (stmt-live? stmt)
                 (not (stmt-cached? stmt)))
        (let ((p (stmt-pointer stmt)))
          (sqlite-remove-statement! (stmt->db stmt) stmt)
          (set-stmt-live?! stmt #f)
          (f p))))))

(define *stmt-map* (make-weak-key-hash-table))
(define (stmt->db stmt)
  (hashq-ref *stmt-map* stmt))

(define stmt-guardian (make-guardian))
(define (pump-stmt-guardian)
  (let ((stmt (stmt-guardian)))
    (if stmt
        (begin
          (sqlite-finalize stmt)
          (pump-stmt-guardian)))))
(add-hook! after-gc-hook pump-stmt-guardian)

(define sqlite-reset
  (let ((reset (pointer->procedure
                int
                (dynamic-func "sqlite3_reset" libsqlite3)
                (list '*))))
    (lambda (stmt)
      (if (stmt-live? stmt)
          (let ((p (stmt-pointer stmt)))
            (set-stmt-reset?! stmt #t)
            (reset p))
          (error "statement already finalized" stmt)))))

(define (assert-live-stmt! stmt)
  (if (not (stmt-live? stmt))
      (error "statement already finalized" stmt)))

(define (assert-live-db! db)
  (if (not (db-open? db))
      (error "database already closed" db)))

(define %sqlite-prepare
  (let ((prepare (pointer->procedure
                  int
                  (dynamic-func "sqlite3_prepare_v2" libsqlite3)
                  (list '* '* int '* '*))))
    (lambda* (db sql #:key cache?)
      (assert-live-db! db)
      (let* ((out-stmt (bytevector->pointer (make-bytevector (sizeof '*) 0)))
             (out-tail (bytevector->pointer (make-bytevector (sizeof '*) 0)))
             (bv (string->utf8 sql))
             (bvp (bytevector->pointer bv))
             (ret (prepare (db-pointer db)
                           bvp
                           (bytevector-length bv)
                           out-stmt
                           out-tail)))
        (if (zero? ret)
            (if (= (bytevector-length bv)
                   (- (pointer-address (dereference-pointer out-tail))
                      (pointer-address bvp)))
                (let ((stmt (make-stmt (dereference-pointer out-stmt) #t #t
                                       cache?)))
                  (stmt-guardian stmt)
                  (hashq-set! *stmt-map* stmt db)
                  stmt)
                (error "input sql has useless tail"
                       (utf8-pointer->string
                        (dereference-pointer out-tail))))
            (check-error db 'sqlite-prepare))))))

(define* (sqlite-prepare db sql #:key cache?)
  (if cache?
      (match (hash-ref (db-stmts db) sql)
        (#f
         (let ((stmt (%sqlite-prepare db sql #:cache? #t)))
           (hash-set! (db-stmts db) sql stmt)
           stmt))
        (stmt
         (sqlite-reset stmt)
         stmt))
      (%sqlite-prepare db sql)))

(define sqlite-bind-parameter-index
  (let ((bind-parameter-index (pointer->procedure
                  int
                  (dynamic-func "sqlite3_bind_parameter_index" libsqlite3)
                  (list '* '*))))
    (lambda (stmt name)
      (assert-live-stmt! stmt)
      (let* ((ret (bind-parameter-index (stmt-pointer stmt)
                                        (string->utf8-pointer name))))
        (if (> ret 0)
            ret
            (begin
              (check-error (stmt->db stmt) 'sqlite-bind-parameter-index)
              (write ret)
              (newline)
              (error "No such parameter" name)))))))

(define key->index
  (lambda (stmt key)
    (cond
     ((string? key) (sqlite-bind-parameter-index stmt key))
     ((symbol? key) (sqlite-bind-parameter-index stmt
                     (string-append ":" (symbol->string key))))
     (else key))))

(define sqlite-bind
  (let ((bind-blob (pointer->procedure
                    int
                    (dynamic-func "sqlite3_bind_blob" libsqlite3)
                    (list '* int '* int '*)))
        (bind-text (pointer->procedure
                    int
                    (dynamic-func "sqlite3_bind_text" libsqlite3)
                    (list '* int '* int '*)))
        (bind-int64 (pointer->procedure
                     int
                     (dynamic-func "sqlite3_bind_int64" libsqlite3)
                     (list '* int int64)))
        (bind-double (pointer->procedure
                      int
                      (dynamic-func "sqlite3_bind_double" libsqlite3)
                      (list '* int double)))
        (bind-null (pointer->procedure
                    int
                    (dynamic-func "sqlite3_bind_null" libsqlite3)
                    (list '* int)))
        (sqlite-transient (make-pointer
                           (bit-extract (lognot 0) 0 (* 8 (sizeof '*))))))
    (lambda (stmt key val)
      (assert-live-stmt! stmt)
      (let ((idx (key->index stmt key))
            (p (stmt-pointer stmt)))
        (cond
         ((bytevector? val)
          (bind-blob p idx (bytevector->pointer val) (bytevector-length val)
                     sqlite-transient))
         ((string? val)
          (let ((bv (string->utf8 val)))
            (bind-text p idx (bytevector->pointer bv) (bytevector-length bv)
                       sqlite-transient)))
         ((and (integer? val) (exact? val))
          (bind-int64 p idx val))
         ((number? val)
          (bind-double p idx (exact->inexact val)))
         ((not val)
          (bind-null p idx))
         (else
          (error "unexpected value" val)))
        (check-error (stmt->db stmt))))))

(define (sqlite-bind-arguments stmt . args)
  "Bind STMT parameters, one after another, to ARGS.
Also bind named parameters to the respective ones."
  (let loop ((i 1)
             (args args))
    (match args
      (()
       #f)
      (((? keyword? kw) value . rest)
       (sqlite-bind stmt (keyword->symbol kw) value)
       (loop i rest))
      ((arg . rest)
       (sqlite-bind stmt i arg)
       (loop (+ 1 i) rest)))))

(define sqlite-column-count
  (let ((column-count
         (pointer->procedure
          int
          (dynamic-pointer "sqlite3_column_count" libsqlite3)
          (list '*))))
    (lambda (stmt)
      (assert-live-stmt! stmt)
      (column-count (stmt-pointer stmt)))))

(define sqlite-column-name
  (let ((column-name
         (pointer->procedure
          '*
          (dynamic-pointer "sqlite3_column_name" libsqlite3)
          (list '* int))))
    (lambda (stmt i)
      (assert-live-stmt! stmt)
      (utf8-pointer->string (column-name (stmt-pointer stmt) i)))))

(define sqlite-column-value
  (let ((value-type
         (pointer->procedure
          int
          (dynamic-pointer "sqlite3_column_type" libsqlite3)
          (list '* int)))
        (value-int
         (pointer->procedure
          int64
          (dynamic-pointer "sqlite3_column_int64" libsqlite3)
          (list '* int)))
        (value-double
         (pointer->procedure
          double
          (dynamic-pointer "sqlite3_column_double" libsqlite3)
          (list '* int)))
        (value-text
         (pointer->procedure
          '*
          (dynamic-pointer "sqlite3_column_text" libsqlite3)
          (list '* int)))
        (value-blob
         (pointer->procedure
          '*
          (dynamic-pointer "sqlite3_column_blob" libsqlite3)
          (list '* int)))
        (value-bytes
         (pointer->procedure
          int
          (dynamic-pointer "sqlite3_column_bytes" libsqlite3)
          (list '* int))))
    (lambda (stmt i)
      (assert-live-stmt! stmt)
      (case (value-type (stmt-pointer stmt) i)
        ((1) ; SQLITE_INTEGER
         (value-int (stmt-pointer stmt) i))
        ((2) ; SQLITE_FLOAT
         (value-double (stmt-pointer stmt) i))
        ((3) ; SQLITE3_TEXT
         (let ((p (value-blob (stmt-pointer stmt) i)))
           (if (null-pointer? p)
               ""
               (utf8->string
                (pointer->bytevector p (value-bytes (stmt-pointer stmt) i))))))
        ((4) ; SQLITE_BLOB
         (let ((p (value-blob (stmt-pointer stmt) i)))
           (if (null-pointer? p)
               (make-bytevector 0)
               (pointer->bytevector p (value-bytes (stmt-pointer stmt) i)))))
        ((5) ; SQLITE_NULL
         #f)))))

(define (sqlite-column-names stmt)
  (let ((v (make-vector (sqlite-column-count stmt))))
    (let lp ((i 0))
      (if (< i (vector-length v))
          (begin
            (vector-set! v i (sqlite-column-name stmt i))
            (lp (1+ i)))
          v))))

(define (sqlite-row stmt)
  (let ((v (make-vector (sqlite-column-count stmt))))
    (let lp ((i 0))
      (if (< i (vector-length v))
          (begin
            (vector-set! v i (sqlite-column-value stmt i))
            (lp (1+ i)))
          v))))

(define sqlite-busy-timeout
  (let ((f (pointer->procedure
            int
            (dynamic-func "sqlite3_busy_timeout" libsqlite3)
            (list '* int))))
    (lambda (db value)
      (assert-live-db! db)
      (let ((ret (f (db-pointer db) value)))
        (when (not (zero? ret))
          (check-error db 'sqlite-busy-timeout))))))

(define sqlite-step
  (let ((step (pointer->procedure
               int
               (dynamic-pointer "sqlite3_step" libsqlite3)
               (list '*))))
    (lambda (stmt)
      (assert-live-stmt! stmt)
      (let ((ret (step (stmt-pointer stmt))))
        (case ret
          ((100) ; SQLITE_ROW
           (sqlite-row stmt))
          ((101) ; SQLITE_DONE
           #f)
          (else
           (check-error (stmt->db stmt))
           (error "shouldn't get here")))))))

(define (sqlite-fold kons knil stmt)
  (assert-live-stmt! stmt)
  (let lp ((seed knil))
    (let ((row (sqlite-step stmt)))
      (if row
          (lp (kons row seed))
          seed))))

(define (sqlite-fold-right kons knil stmt)
  (assert-live-stmt! stmt)
  (let lp ()
    (let ((row (sqlite-step stmt)))
      (if row
          (kons row (lp))
          knil))))

(define (sqlite-map proc stmt)
  (map proc
       (reverse! (sqlite-fold cons '() stmt))))
