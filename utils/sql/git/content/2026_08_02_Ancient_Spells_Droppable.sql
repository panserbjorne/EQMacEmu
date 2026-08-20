UPDATE items SET nodrop = "-1" WHERE id IN (SELECT id FROM items WHERE NAME LIKE '%Ancient:%')

/* SELECT * FROM `items` WHERE NAME LIKE '%Ancient:%' 
returns 16 rows, spells included:
Ancient: High Priest's Bulwark
Ancient: Legacy of Blades
Ancient: Starfire of Ro
Ancient: Feral Avatar
Ancient: Scourge of Nife
Ancient: Master of Death
Ancient: Lifebane
Ancient: Destruction of Ice
Ancient: Greater Concussion
Ancient: Shock of Sun
Ancient: Burnout Blaze
Ancient: Eternal Rapture
Ancient: Chaotic Visions
Ancient: Gift of Aegolism
Ancient: Lcea's Lament
Ancient: Lullaby of Shadow  */
