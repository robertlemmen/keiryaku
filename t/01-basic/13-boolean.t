#t
#f
'#t
'#f
(not #t)
(not #f)
(not 1)
(not 0)
(not (list 3))
(not (list))
(not '())
(not (not #t))
(not (not #f))
(boolean? #t)
(boolean? #f)
(boolean? 1)
(boolean? '())
(boolean? '(#t #f))
(boolean? (not 3))
(boolean=? #t #t)
(boolean=? #t #t #t)
(boolean=? #t #t #f)
(boolean=? #t #f #t)
(boolean=? #f #t #t)
(boolean=? #f #f)
(boolean=? #f #f #f)
(boolean=? #f #t)
(boolean=? #f #t #f)
(boolean=? #f (not #f))
===
#t
#f
#t
#f
#f
#t
#f
#f
#f
#f
#f
#t
#f
#t
#t
#f
#f
#f
#t
#t
#t
#f
#f
#f
#t
#t
#f
#f
#f
