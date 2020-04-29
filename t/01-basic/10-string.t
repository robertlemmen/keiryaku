""
"test"
(string? "test")
(string? "1231232132132132132313231")
(string? 123)
(string-length "")
(string-length "test")
(string-length "1234567890")
(string=? "ofenrohr" "ofenrohr")
(string=? "" "test")
"a\"b\tc\|d\\"
"A\nB"
===
""
"test"
#t
#t
#f
0
4
10
#t
#f
"a"b	c|d\"
"A
B"
