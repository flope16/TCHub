# Notes techniques DTU 60.11 – Bouclage ECS et pertes thermiques

Ce document résume les points clés du DTU 60.11 à garder en tête pour le dimensionnement dynamique d'un réseau ECS bouclé. Il ne remplace pas la lecture du texte officiel mais fournit des rappels chiffrés et des formules pratiques pour le calcul en ligne.

## Exigences de température
- Température minimale en tout point du réseau de distribution et de retour : **≥ 55 °C** pour limiter le risque de légionelles.
- Écart typique aller/retour sur un bouclage d'ECS collective : **3 à 5 °C**. Au-delà, le débit de recyclage ou l'isolation doivent être revus.

## Vitesses recommandées
- Tuyauteries acier/copper en distribution ECS : viser **0,5 à 1,0 m/s** (limiter bruit et érosion tout en réduisant le temps d'attente).
- Tuyauteries d'eau froide : **0,8 à 1,5 m/s** (selon matériaux et pression disponible).

## Pertes linéiques et choix de diamètre
1. **Perte thermique linéique** (W/m) pour un tronçon isolé :
   \[
   \dot Q_l = U \times \pi \times D_{ext} \times (T_{eau} - T_{amb})
   \]
   - *U* : coefficient global d'échange (W/m².K) dépendant de l'isolant (typiquement 0,5 à 1,0 W/m².K pour 20–30 mm de laine élastomère).
   - *D_ext* : diamètre extérieur de l'isolant (m).
   - *T_eau*, *T_amb* : températures eau et ambiance.

2. **Chute de température sur un tronçon** de longueur *L* parcouru par un débit massique \(\dot m = \rho \times Q\) :
   \[
   \Delta T = \frac{\dot Q_l \times L}{\dot m \times c_p}
   \]
   - Avec \(\rho \approx 1000\,\text{kg/m}^3\) et \(c_p \approx 4180\,\text{J/kg.K}\).
   - À 55–60 °C, une perte de **~120 W sur 10 m** avec \(U = 0{,}8\,\text{W/m².K}\), \(D_{ext} = 0{,}06\,\text{m}\), \(T_eau=60\,°C, T_{amb}=20\,°C\) est plausible :
     - \(\dot Q_l \approx 0{,}8 \times \pi \times 0{,}06 \times 40 \approx 6\,\text{W/m}\)
     - Sur 10 m : \(\approx 60\,\text{W}\). Si l'isolant est plus faible (U≈1,6), on atteint ~120 W. La cohérence dépend donc fortement de l'isolant choisi.

3. **Ajuster le débit** : pour garantir \(T_{sortie} \ge 55\,°C\), imposer un débit minimal \(Q_{min}\) tel que :
   \[
   Q_{min} = \frac{\dot Q_l \times L}{\rho \times c_p \times (T_{entree}-55)}
   \]
   - Si le débit calculé est supérieur au plafond de vitesse (1 m/s), augmenter le diamètre ou améliorer l'isolation.

## Mélanges et branches de retour
- Au point de bouclage, calculer la température mixte par bilan d'énergie :
  \[
  T_{mix} = \frac{\sum_i Q_i T_i}{\sum_i Q_i}
  \]
- Appliquer ensuite les pertes thermiques sur la branche de retour avec le même schéma que pour l'aller.

## Contrôles à implémenter dans le simulateur
- Vérifier pour chaque tronçon :
  - \(T_{sortie} \ge 55\,°C\) (sinon signaler débit insuffisant ou isolation trop faible).
  - **Écart aller/retour** global \(\le 5\,°C\) par boucle.
  - **Vitesse** \(\le 1{,}0\,\text{m/s}\) en ECS.
- Signaler les tronçons où \(\Delta T\) dépasse **1 °C / 10 m** avec isolant standard, car cela indique soit un isolant faible, soit un débit trop bas.

## Paramètres par défaut suggérés
- \(U = 0{,}8\,\text{W/m².K}\) (isolant 25–30 mm, µ élevé).
- \(T_{amb} = 20\,°C\) en locaux chauffés, **10 °C** en locaux non chauffés.
- \(T_{entree} = 60\,°C\) côté préparateur, objectif \(T_{retour} = 55–57\,°C\).

## Références utiles
- **NF DTU 60.11** : règles de calcul des installations de plomberie sanitaire et d'eaux glacées.
- **Guide pratique CSTB – Réseaux ECS bouclés** : conseils sur isolation et prévention légionelles.
- **Arrêté du 30/11/2005** (arrêté « légionelles ») : température minimale de 55 °C en distribution.
