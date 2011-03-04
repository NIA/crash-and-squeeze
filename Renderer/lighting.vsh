vs_2_0
dcl_position v0
dcl_color0 v1
dcl_color1 v2 ; cluster index
dcl_normal v3
dcl_color3 v4 ; clusters number

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; c0 - c3 is view matrix           ;;
;; c12 is directional light vector  ;;
;; c13 is directional light color   ;;
;; c14 is diffuse coefficient       ;;
;; c15 is ambient light color       ;;
;; c16 is point light color         ;;
;; c17 is point light position      ;;
;; c18 are attenuation constants    ;;
;; c19 is specular coefficient      ;;
;; c20 is specular constant 'f'     ;;
;; c21 is eye position              ;;
;; c22 - c25 is pos.*rot. matrix    ;;
;; c26-c41 are initial cluster c.m. ;;
;; c42-c105 are cluster matrices    ;;
;; c106-c110 are 4x4 zero matrix    ;;
;; c120-c183 are cluster normal mxs ;;
;; c184-c187 are 4x4 zero matrix    ;;
;;                                  ;;
;; c111 is constant 1.0f            ;;
;; c255 is constant 0.0f            ;;
;;                                  ;;
; ?r0  is attenuation               ;;
; !r1  is transformed vertex        ;;
;; r2  is r (for specular)          ;;
;; r3  is temp                      ;;
;; r4  is light intensity           ;;
; !r5  is cos(theta)                ;;
; !r6  is result color              ;;
;; r7  is temp                      ;;
;; r8  is cos(phi) (specular)       ;;
; !r9  is normalized eye (v)        ;;
; !r10 is transformed normal        ;;
;; r11 is direction vector          ;;
;;                                  ;;
;; r0 is 1/current radius           ;;
;; r1 is a quotient                 ;;
;; r2 is a vertex after morphing    ;;
;;                                  ;;
; ?r0  is attenuation               ;;
; !r1  is transformed vertex        ;;
;; r2  is r (for specular)          ;;
;; r3  is temp                      ;;
;; r4  is light intensity           ;;
; !r5  is cos(theta)                ;;
; !r6  is result color              ;;
;; r7  is temp                      ;;
;; r8  is cos(phi) (specular)       ;;
; !r9  is normalized eye (v)        ;;
; !r10 is transformed normal        ;;
;; r11 is direction vector          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def c255, 0.0, 0.0, 0.0, 0.0
def c111, 1.0, 1.0, 1.0, 1.0 ;constant one
def c114, 4.0, 4.0, 4.0, 4.0 ;constant 4

def c115, 0.5, 0.5, 0.5, 0.5
def c112, 0.125, 0.125, 0.125, 0.125 ;constant 1/8

; - - - - - - - - - -  position  - - - - - - - - - - - - - -;
; #1 -> r1, r9
mova a0.x, v2.x         ; a0.x = cluster #1 index
add r4, v0, -c[26+a0.x] ; r1 = vertex.pos - cluster.center
mul r3.x, v2.x, c114    ; a0.x = 4 * cluster #1 index
mova a0.x, r3.x
m4x4 r1, r4, c[42+a0.x] ; r1 = [cluster.matrix] * r4
m4x4 r9, v3,c[120+a0.x] ; r9 = [cluster.matrix.inv.trans] * vertex.normal
; #2 -> r2, r10 -> r1, r9
mova a0.x, v2.y         ; a0.x = cluster #2 index
add r4, v0, -c[26+a0.x] ; r4 = vertex.pos - cluster.center
mul r3.x, v2.y, c114    ; a0.x = 4 * cluster #2 index
mova a0.x, r3.x
m4x4 r2, r4, c[42+a0.x] ; r2 = [cluster.matrix] * r4
m4x4 r10,v3,c[120+a0.x] ; r10 = [cluster.matrix.inv.trans] * vertex.normal
add r1, r1, r2
add r9, r9, r10
; #3 -> r2, r10 -> r1, r9
mova a0.x, v2.z         ; a0.x = cluster #3 index
add r4, v0, -c[26+a0.x] ; r4 = vertex.pos - cluster.center
mul r3.x, v2.z, c114    ; a0.x = 4 * cluster #3 index
mova a0.x, r3.x
m4x4 r2, r4, c[42+a0.x] ; r2 = [cluster.matrix] * r4
m4x4 r10,v3,c[120+a0.x] ; r10 = [cluster.matrix.inv.trans] * vertex.normal
add r1, r1, r2
add r9, r9, r10
; #4 -> r2, r10 -> r1, r9
mova a0.x, v2.w         ; a0.x = cluster #4 index
add r4, v0, -c[26+a0.x] ; r4 = vertex.pos - cluster.center
mul r3.x, v2.w, c114    ; a0.x = 4 * cluster #4 index
mova a0.x, r3.x
m4x4 r2, r4, c[42+a0.x] ; r2 = [cluster.matrix] * r4
m4x4 r10,v3,c[120+a0.x] ; r10 = [cluster.matrix.inv.trans] * vertex.normal
add r1, r1, r2
add r9, r9, r10
; sum -> average
rcp r3, v4.x            ; r3 = 1/clusters_num
mul r2, r1, r3          ; r2 = r1 / clusters_num = deformed position
mul r9, r9, r3          ; r9 = r9 / clusters_num = deformed normal
; shift and rotation
m4x4 r1, r2, c22        ; r1 = [position and rotation] * r2

; - - - - - - - - - -  normals  - - - - - - - - - - - - - - ;
m4x4 r10, r9, c22       ; position and rotation
dp3 r2, r10, r10        ; r2 = |normal|**2
rsq r7, r2.x            ; r7 = 1/|normal|
mul r10, r10, r7.x      ; normalize r10

; calculating normalized v
add r9, c21, -r1       ; r9 = position(eye) - position(vertex)
dp3 r0, r9, r9         ; r0 = distance**2
rsq r7, r0.x           ; r7 = 1/distance
mul r9, r9, r7.x       ; normalize r9
;;;;;;;;;;;;;;;;;;;;; Directional ;;;;;;;;;;;;;;;;;;;;;;;;;;;
dp3 r5, c12, r10        ; r5 = cos(theta)
; - - - - - - - - - - - diffuse - - - - - - - - - - - - - - ;
mul r4, c13, r5.x       ; r4 = I(direct)*cos(theta)
mul r4, r4, c14.x       ; r4 *= coef(diffuse)

max r6, r4, c255        ; if some color comp. < 0 => make it == 0
; - - - - - - - - - - - specular - - - - - - - - - - - - - -;
; calculating r:
mul r2, r10, r5.x     ; r2 = 2*(l, n)*n
add r2, r2, r2        ; r2 = 2*(l, n)*n
add r2, r2, -c12      ; r2 = 2*(l, n)*n - l
; calculating cos(phi)**f
dp3 r8, r2, r9          ; r8 = cos(phi)
max r8, r8, c255        ; if cos < 0, let it = 0
mov r7.y, r8.x
mov r7.w, c20.x
lit r8, r7              ; r8.z = cos(phi)**f

mul r4, c13, r8.z       ; r4 = I(direct)*cos(phi)**f
mul r4, r4, c19.x       ; r4 *= coef(specular)

max r4, r4, c255        ; if some color comp. < 0 => make it == 0
add r6, r6, r4

;;;;;;;;;;;;;;;;;;;;;;;; Point ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; calculating normalized direction vector
add r11, c17, -r1       ; r11 = position(point) - position(vertex)
dp3 r2, r11, r11        ; r2 = distance**2
rsq r7, r2.x            ; r7 = 1/distance
mul r11, r11, r7.x      ; normalize r11
; calculating cos(theta)
dp3 r5, r11, r10        ; r5 = cos(theta)
; calculating attenuation
dst r2, r2, r7          ; r2 = (1, d, d**2, 1/d)
dp3 r0, r2, c18         ; r0 = (a + b*d + c*d**2)
rcp r0, r0.x            ; r0 = attenuation coef
; - - - - - - - - - - - diffuse - - - - - - - - - - - - - - ;
mul r4, c16, r5.x       ; r4 = I(point)*cos(theta)
mul r4, r4, c14.x        ; r4 *= coef(diffuse)
mul r4, r4, r0.x        ; r4 *= attenuation

max r4, r4, c255        ; if some color comp. < 0 => make it == 0
add r6, r6, r4
; - - - - - - - - - - - specular - - - - - - - - - - - - - -;
; calculating r:
mul r2, r10, r5.x   ; r2 = (l, n)*n
add r2, r2, r2      ; r2 = 2*(l, n)*n
add r2, r2, -r11    ; r2 = 2*(l, n)*n - l
; calculating cos(phi)**f
dp3 r8, r2, r9          ; r8 = cos(phi)
max r8, r8, c255        ; if cos < 0, let it = 0
mov r7.y, r8.x
mov r7.w, c20.x
lit r8, r7              ; r8.z = cos(phi)**f

mul r4, c16, r8.z       ; r4 = I(point)*cos(phi)**f
mul r4, r4, c19.x       ; r4 *= coef(specular)
mul r4, r4, r0.x        ; r4 *= attenuation

max r4, r4, c255        ; if some color comp. < 0 => make it == 0
add r6, r6, r4

;;;;;;;;;;;;;;;;;;;;;;; Ambient ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
add r6, r6, c15         ; r6 += I(ambient)

mul r7, v4.x, c112
add r7, r7, c115
mov r7.a, c111
mul r7, r7, v1          ; r7 is vertex color with pattern applied

;;;;;;;;;;;;;;;;;;;;;;;; Results ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
m4x4 oPos, r1, c0
mul oD0, r7, r6
