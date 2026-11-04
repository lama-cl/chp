Expected C3a behavior:
- Parameter-side import:
    f::x includes g⌜char * ... int *⌝g-style imported parameter views.
- Pointer-use import:
    f::x includes imported deref-readwrite through int *.
- Behavior should not be Unknown; expected concrete write evidence should be
  visible in pointer behavior metrics.
- Return import:
    f's return should carry g's return behavior and alias $0.
