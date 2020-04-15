(equal? #t #t)
(equal? #f #f)
(equal? #f '#f)

(equal? #f (not #t))
(equal? #f (not 3))
(equal? #f (not (list 3)))
(equal? #t (not #f))
(equal? #f (not '()))
(equal? #f (not (list)))
(equal? #f (not 'nil))

(boolean? #f)
(boolean? 0)
(boolean? '())

(boolean=? #t #t)
(boolean=? #f #f)
(boolean=? #t #f)
(boolean=? #f #f #f)
(boolean=? #t #t #f)
===
#t
#t
#t

#t
#t
#t
#t
#t
#t
#t

#t
#f
#f

#t
#t
#f
#t
#f
