def s: map(select((if type=="string" then . else .name end) as $n | $n!="qtbase" and $n!="qt5compat"));
(if has("dependencies") then .dependencies |= s else . end)
| (if has("features") then .features |= map_values(if has("dependencies") then .dependencies |= s else . end) else . end)
