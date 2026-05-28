---
date: 2026-05-22
type: reference
tags: [dspx, cryostat, dimensions, geometry]
---

# DSPX Cryostat — Canonical Dimensions

Confirmed DSPX cryostat geometry values and inter-screen gap cascade.

> **Canonical source:** `src/Geometry/CryostatBuilder.cc` (implementation)  
> **Companion note:** [[01_Implementation_Plans]] (merge status)

---

## Plate Dimensions (CryoConcept2020)

| Plate | Outer radius (mm) | Thickness (mm) | Z position (mm) |
|-------|-------------------|----------------|------------------|
| Plate1K | 142.0 | 3.0 | 182.7 |
| Plate4K | 161.5 | 6.0 | 316.2 |
| Plate50K | 180.0 | 6.0 | 525.7 |

> z=0 at Plate10mK bottom face. Z values = centre of each plate.

## Screen Dimensions (Bucket cups)

| Screen | h (mm) | Driven gap from plate below |
|--------|--------|---------------------------|
| Screen1K | 483.0 | top flange aligned to Plate1K bottom |
| Screen4K | 623.0 | top flange aligned to Plate4K bottom |
| Screen50K | 840.0 | top flange aligned to Plate50K bottom |
| OVC | 1030.0 | top rim aligned to bride mouth |

## ASCII Cross-Section

```
==============================================================================
  DSPX Cryostat -- complete cross-section (corrected: OVC top = bride mouth)
  z=0 at Plate10mK bottom face.  +z up.  r scale: 1ch=10mm  z: 1row=10mm
  Screens = Bucket (cup); 1K, 4K, and 50K have top flanges. OVC =
  stainless-steel Bucket (top rim at bride mouth). Bride = HexBride.
==============================================================================

  r(mm):  0   50  100 125 142 152 162 170 180 191 196 208
  z(mm)

    788    |
    778    |
    768    |                  ═══                   <- Bride closed top  z=765.4mm
    758    |                  ║ ║
    748    |                  ║ ║
    738    |                  ║ ║
    728    |                  ║ ║
    718    |                  ║ ║
    708    |                  ║ ║
    698    |                  ║ ║
    688    |                  ║ ║
    678    |                  ║ ║
    668    |                  ║ ║
    658    |                  ╙─╜                   <- Bride open mouth = OVC top rim  z=654.4mm
    648    |                  ║║
    638    |                  ║║
    628    |                  ║║                   <- OVC top ring bottom  z=627.4mm
    618    |                  ║║
    608    |                  ║║
    598    |                  ║║
    588    |                  ║║
    578    |                  ║║
    568    |                  ║║
    558    |                  ║║
    548    |                  ║║
    538    |                  ║║
    528    |##################║║
    518    |##################║║                    <- Screen50K top flange / Plate50K bottom
    508    |                │ ║║
    498    |                │ ║║
    488    |                │ ║║
    478    |                │ ║║
    468    |                │ ║║
    458    |                │ ║║
    448    |                │ ║║
    438    |                │ ║║
    428    |                │ ║║
    418    |                │ ║║
    408    |                │ ║║
    398    |                │ ║║
    388    |                │ ║║
    378    |                │ ║║
    368    |                │ ║║
    358    |                │ ║║
    348    |                │ ║║
    338    |                │ ║║
    328    |                │ ║║
    318    |################│ ║║                    <- Screen4K top flange / Plate4K bottom
    308    |              │ │ ║║
    298    |              │ │ ║║
    288    |              │ │ ║║
    278    |              │ │ ║║
    268    |              │ │ ║║
    258    |              │ │ ║║
    248    |              │ │ ║║
    238    |              │ │ ║║
    228    |              │ │ ║║
    218    |              │ │ ║║
    208    |              │ │ ║║
    198    |              │ │ ║║
    188    |##############│ │ ║║
    178    |##############│ │ ║║                    <- Screen1K top flange / Plate1K bottom
    168    |            │ │ │ ║║
    158    |            │ │ │ ║║
    148    |            │ │ │ ║║
    138    |            │ │ │ ║║
    128    |            │ │ │ ║║
    118    |            │ │ │ ║║
    108    |            │ │ │ ║║
     98    |            │ │ │ ║║
     88    |            │ │ │ ║║
     78    |############│ │ │ ║║                    <- Plate100mK  r=125  e=3mm
     68    |            │ │ │ ║║
     58    |            │ │ │ ║║
     48    |            │ │ │ ║║
     38    |            │ │ │ ║║
     28    |            │ │ │ ║║
     18    |            │ │ │ ║║
      8    |############│ │ │ ║║
     -2    |############│ │ │ ║║                    <- Plate10mK  r=125  e=4mm
    -12    |            │ │ │ ║║
    -22    |            │ │ │ ║║
    -32    |            │ │ │ ║║
    -42    |            │ │ │ ║║
    -52    |            │ │ │ ║║
    -62    |            │ │ │ ║║
    -72    |            │ │ │ ║║
    -82    |            │ │ │ ║║
    -92    |            │ │ │ ║║
   -102    |            │ │ │ ║║
   -112    |            │ │ │ ║║
   -122    |            │ │ │ ║║
   -132    |            │ │ │ ║║
   -142    |            │ │ │ ║║
   -152    |            │ │ │ ║║
   -162    |            │ │ │ ║║
   -172    |            │ │ │ ║║
   -182    |            │ │ │ ║║
   -192    |            │ │ │ ║║
   -202    |            │ │ │ ║║
   -212    |            │ │ │ ║║
   -222    |            │ │ │ ║║
   -232    |            │ │ │ ║║
   -242    |            │ │ │ ║║
   -252    |            │ │ │ ║║
   -262    |            │ │ │ ║║
   -272    |            │ │ │ ║║
   -282    |            │ │ │ ║║
   -292    |            │ │ │ ║║
   -302    |════════════╝ │ │ ║║                    <- Screen1K disk bottom  z=-303.8mm
   -312    |══════════════╝ │ ║║                    <- Screen4K disk bottom  z=-311.8mm
   -322    |                │ ║║
   -332    |                │ ║║
   -342    |                │ ║║
   -352    |════════════════╝ ║║                    <- Screen50K disk bottom  z=-347.3mm
   -362    |                  ║║
   -372    |                  ║║
   -382    |══════════════════╚╝                    <- ScreenOVC disk bottom  z=-381.9mm
   -392    |

==============================================================================
  Confirmed dimensions -- all values in mm
==============================================================================

  PLATES  (FilledTube, copper)
  ┌─────────┬────────┬──────┬──────────────────────┬──────────┐
  │ Plate   │ r (mm) │ e mm │ gap top→bot next mm  │ z_bot mm │
  ├─────────┼────────┼──────┼──────────────────────┼──────────┤
  │ 10mK    │  125.0 │    4 │  70.20 → bot 100mK   │     0.00 │
  │ 100mK   │  125.0 │    3 │ 104.00 → bot 1K      │    74.20 │
  │ 1K      │  142.0 │    3 │ 129.00 → bot 4K      │   181.20 │
  │ 4K      │  161.5 │    6 │ 203.50 → bot 50K     │   313.20 │
  │ 50K     │  180.0 │    6 │ 125.70 → bride mouth │   522.70 │
  └─────────┴────────┴──────┴──────────────────────┴──────────┘

  SCREENS  (Bucket: hollow cylinder + solid bottom disk, copper)
  Heights DERIVED.  OVC top rim = bride open mouth (CORRECTED).
  ┌──────────┬────────┬──────┬───────────┬──────────┬──────────┐
  │ Screen   │ ri(mm) │ e mm │ pos (mm)  │  h (mm)  │ disk bot │
  ├──────────┼────────┼──────┼───────────┼──────────┼──────────┤
  │ 1K       │  130.5 │ 1.02 │   -302.78 │   483.98 │  -303.80 │
  │ 4K       │  150.0 │    2 │   -309.80 │   623.00 │  -311.80 │
  │ 50K      │  167.5 │    2 │   -317.30 │   840.00 │  -319.30 │
  │ OVC      │  185.5 │ 6.28 │   -375.60 │  1030.00 │  -381.88 │
  └──────────┴────────┴──────┴───────────┴──────────┴──────────┘

  Gap cascade:
    Plate10mK bot → Screen1K  inner bot : 302.8 mm  (from measured 1K height)
    Screen1K  outer bot → Screen4K  inner bot :  -6.0 mm  (4K bottom is lower; larger-radius shell)
    Screen4K  outer bot → Screen50K inner bot :   5.5 mm  (from measured 4K/50K heights)
    Screen50K outer bot → OVC inner bot       :  56.3 mm  (from measured OVC/50K heights)

  1K measured dimensions:
    inner diameter                     = 261.0 mm
    wall thickness                     = 0.04 in = 1.016 mm
    tub outer diameter                 = 263.032 mm
    flange outer diameter              = 282.0 mm
    flange stack thickness             = 0.505 in = 12.827 mm
    top aluminum ring thickness        = 0.09 in = 2.286 mm
    mu-metal flange thickness          = 0.04 in = 1.016 mm
    bottom aluminum ring thickness     = 0.375 in = 9.525 mm
    top flange → bottom tub surface     = 485.0 mm
    top flange → top tub bottom surface = 483.984 mm
    top flange z                       = 181.20 mm (= Plate1K bottom)
    inner bottom z                     = -302.784 mm
    outer bottom z                     = -303.80 mm
    material                           = MuMetal bucket + middle flange, aluminum top/bottom rings
    visual colour                      = teal, distinct from copper, aluminum, and stainless steel
    MuMetal composition                = 80% Ni, 5% Mo, 15% Fe by mass
    MuMetal density used               = 8.7 g/cm^3
    aluminum ring visual colour        = blue

  4K measured dimensions:
    inner diameter                     = 300.0 mm
    wall thickness                     = 2.0 mm
    tub outer diameter                 = 304.0 mm
    flange outer diameter              = 325.0 mm
    flange thickness                   = 8.0 mm
    top flange → top tub bottom surface = 623.0 mm
    top flange → bottom tub surface     = 625.0 mm
    top flange z                       = 313.20 mm (= Plate4K bottom)
    inner bottom z                     = -309.80 mm
    outer bottom z                     = -311.80 mm

  50K measured dimensions:
    inner diameter                     = 335.0 mm
    wall thickness                     = 2.0 mm
    tub outer diameter                 = 339.0 mm
    flange outer diameter              = 360.0 mm
    flange thickness                   = 13.0 mm
    top flange → top tub bottom surface = 840.0 mm
    top flange → bottom tub surface     = 842.0 mm
    top flange z                       = 522.70 mm (= Plate50K bottom)
    inner bottom z                     = -317.30 mm
    outer bottom z                     = -319.30 mm

  OVC measured dimensions:
    inner diameter       = 371.0 mm
    outer circumference  = 1205.0 mm
    outer radius         = 191.78 mm
    wall thickness       = 6.28 mm
    material             = stainless steel
    top rim height       = 2100.0 mm above lab floor when globalOffset z = 0

  OVC TOP RING  (separate stainless-steel annulus)
    inner radius = 191.78 mm  (= OVC outer radius)
    outer radius = 208.0 mm   (= 416.0 mm outer diameter / 2)
    height       = 27.0 mm
    z top        = 654.40 mm  (= OVC top rim / bride open mouth)
    z bottom     = 627.40 mm

  BRIDE  (HexBride: G4Polyhedra, stainless steel)
    bore ri             = 187.0 mm
    inner height (h)    =  97.0 mm
    top disk thickness  =  14.0 mm  (= 111 - 97)
    total height        = 111.0 mm
    outer hex flat-to-flat = 416.0 mm
    inradius (apothem)     = 208.0 mm
    circumradius           = 240.2 mm  <- G4Polyhedra rOuter
    z open mouth = 654.40 mm  (= OVC top rim)
    z closed top = 765.40 mm
    rotation     = 60 deg around z-axis

  G4Polyhedra z-planes (origin = open mouth, +z toward closed top):
    zPlanes = [   0,    97,    97,   111 ]  mm
    rInner  = [ 187,   187,     0,     0 ]  mm
    rOuter  = [ 240.2, 240.2, 240.2, 240.2] mm
    phiStart=0, phiTotal=360deg, numSide=6
    G4RotationMatrix bride_rot = rotateZ(60*deg)
    G4ThreeVector bride_pos = {0, 0, 654.40*mm}

(Approximate ASCII; use canonical value table above for exact dimensions.)
```

## BuildScreen Anatomy (Not used here, but relates to the Ricochet cryostat)

```
BuildScreen(name, innerRadius, thickness, height, screen_e, flange_h)

    <- screen_radius ->  se  <- flange_e ->

PLATE (e.g. Plate1K)       [▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓]          <- Plate_e
───────────────────────────┬──┬──────────────────  z = position + bot_h + mid_h + top_h
                            |  |                    <- screen top, pinned to plate bottom face
                            |  |
                         ^  |  |
                      top_h |  |
                         |  |  |
                         v  |  |════════╡           <- FlangeTop    height = flange_h
                            |  |
                         ^  |  |
                      mid_h |  |
                         |  |  |
                            |  |════════╡           <- FlangeMid    height = 2*flange_h
                            |  |════════╡             (double-thick)
                         v  |  |
                         ^  |  |
                      bot_h |  |
                         |  |  |
                            |  |════════╡           <- FlangeBot    height = 2*flange_h
                            |  |════════╡             (double-thick)
                         v  |  |
───────────────────────────┴──┴──────────────────  z = position  (inner bottom of screen)
═══════════════════════════╧══╧═══════════════════  Bottom disk   thickness = screen_e
══════════════════════════════════════════════════
```
