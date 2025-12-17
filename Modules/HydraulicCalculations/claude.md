# Module de Calculs Hydrauliques - Documentation pour Claude

## Vue d'ensemble

Module de calcul et dimensionnement de réseaux hydrauliques avec interface graphique interactive permettant de dessiner des schémas de tuyauterie, placer des appareils sanitaires, et calculer les diamètres de canalisations.

## Architecture

### Fichiers principaux

```
HydraulicCalculations/
├── HydraulicCalculationsWindow.h/cpp  # Fenêtre principale et logique métier
├── HydraulicSchemaView.h/cpp          # Vue graphique interactive (QGraphicsView)
├── GraphicPipeSegment.h/cpp           # Représentation graphique d'un tronçon
├── FixturePoint.h/cpp                 # Point de puisage (appareil sanitaire)
├── PipeCalculator.h/cpp               # Moteur de calcul hydraulique
└── claude.md                          # Ce fichier
```

### Hiérarchie des classes

```
QDialog
└── HydraulicCalculationsWindow
    ├── HydraulicSchemaView (QGraphicsView)
    │   └── QGraphicsScene
    │       ├── GraphicPipeSegment (QGraphicsItemGroup) [multiple]
    │       └── FixturePoint (QGraphicsItemGroup) [multiple]
    └── PipeCalculator
```

### Flux de données

```
1. Utilisateur dessine → HydraulicSchemaView
2. Création segment → GraphicPipeSegment créé avec ID
3. Données stockées → networkSegments (vector<NetworkSegment>)
4. Calcul demandé → PipeCalculator.calculateNetwork()
5. Résultats copiés → networkSegments (par ID)
6. Affichage mis à jour → GraphicPipeSegment.updateResultsDisplay()
```

## Gestion de la mémoire - CRITIQUE

### ⚠️ PIÈGE N°1: Dangling Pointers (RÉSOLU)

**ANCIEN CODE (BUGUÉ):**
```cpp
// ❌ NE JAMAIS FAIRE ÇA
class GraphicPipeSegment {
    NetworkSegment* segmentData;  // Pointeur brut vers element du vector
};

networkSegments.push_back(newSeg);
auto* graphic = new GraphicPipeSegment(&networkSegments.back());
// Si networkSegments se réalloue → graphic->segmentData INVALIDE!
```

**CODE ACTUEL (CORRECT):**
```cpp
// ✅ STOCKAGE PAR ID
class GraphicPipeSegment {
    std::string segmentId;  // ID au lieu de pointeur
    std::string getSegmentId() const { return segmentId; }
};

// Recherche par ID quand nécessaire
NetworkSegment* seg = findSegmentById(graphic->getSegmentId());
```

### Règles de gestion mémoire

1. **JAMAIS stocker de pointeurs vers éléments d'un `std::vector`** qui peut se réallouer
2. **TOUJOURS** utiliser des IDs pour les références persistantes
3. **TOUJOURS** passer les données en paramètre aux méthodes d'update
4. Les `GraphicPipeSegment` sont gérés par Qt (QGraphicsScene les possède)

### Exemple de lookup par ID

```cpp
// Dans HydraulicCalculationsWindow
NetworkSegment* HydraulicCalculationsWindow::findSegmentById(const std::string& id) {
    for (auto& seg : networkSegments) {
        if (seg.id == id) return &seg;
    }
    return nullptr;
}

// Utilisation
auto* graphic = currentSelectedSegment;
NetworkSegment* data = findSegmentById(graphic->getSegmentId());
if (data) {
    graphic->updateDisplay(data);
}
```

## Patterns de mise à jour d'affichage

### Pattern 1: Mise à jour d'un segment individuel

```cpp
// Après modification d'un segment
std::string segId = graphicSegment->getSegmentId();
NetworkSegment* segData = findSegmentById(segId);
if (segData) {
    graphicSegment->updateDisplay(segData);
}
```

### Pattern 2: Mise à jour de tous les segments après calcul

```cpp
// Utiliser lambda pour lookup
schemaView->updateSegmentResults([this](const std::string& id) {
    return this->findSegmentById(id);
});
```

### Pattern 3: Création d'un nouveau segment

```cpp
// 1. Créer les données
NetworkSegment newSeg;
newSeg.id = "seg_" + std::to_string(networkSegments.size() + 1);
networkSegments.push_back(newSeg);

// 2. Créer l'élément graphique (passe pointeur TEMPORAIRE pour extraire l'ID)
GraphicPipeSegment* graphic = schemaView->addSegment(
    &networkSegments.back(), startPt, endPt
);

// 3. Mettre à jour l'affichage avec lookup par ID
NetworkSegment* segData = findSegmentById(newSeg.id);
if (segData) {
    graphic->updateDisplay(segData);
}
```

## Validation de hiérarchie parent-enfant

### Validations implémentées (dans performCalculations)

```cpp
// Vérifie avant le calcul:
1. Existence du parent référencé
2. Pas d'auto-référence (segment parent de lui-même)
3. Pas de boucles circulaires (A→B→C→A)
4. Profondeur maximale < 100 niveaux
```

### Messages d'erreur en français

Les validations lancent des `std::runtime_error` avec messages explicites:
- "Le segment 'X' référence un parent inexistant 'Y'"
- "Le segment 'X' se référence lui-même comme parent (boucle détectée)"
- "Boucle circulaire détectée dans la hiérarchie du segment 'X'"
- "Hiérarchie trop profonde détectée (> 100 niveaux) pour le segment 'X'"

## Calcul hydraulique

### Flux de calcul

```cpp
1. updateNetworkSegmentsData()  // Sync fixtures depuis graphiques
2. performCalculations()         // Valide hiérarchie + calcule
3. calculator.calculateNetwork() // Moteur de calcul
4. Copie résultats par ID        // NE PAS remplacer le vector!
5. updateSegmentResults()        // Affichage
```

### ⚠️ PIÈGE N°2: Remplacement du vector

**❌ FAUX:**
```cpp
networkSegments = calculatedSegments;  // Invalide tous les IDs!
```

**✅ CORRECT:**
```cpp
// Copier UNIQUEMENT les résultats par ID
for (const auto& calculated : calculatedSegments) {
    for (auto& seg : networkSegments) {
        if (seg.id == calculated.id) {
            seg.result = calculated.result;
            seg.inletPressure = calculated.inletPressure;
            seg.outletPressure = calculated.outletPressure;
            break;
        }
    }
}
```

## Gestion des événements utilisateur

### Mode d'interaction

```cpp
enum class InteractionMode {
    Select,      // Sélectionner segments/fixtures
    AddSegment,  // Dessiner nouveau segment (2 clics)
    AddFixture,  // Placer appareil sur segment
    Pan          // Déplacer la vue
};
```

### Désactivation d'événements indésirables

```cpp
// Échap ne ferme pas la fenêtre
void keyPressEvent(QKeyEvent* event) override {
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
        return;
    }
}

// Molette ne modifie pas les spinbox
QObject* wheelFilter = new SpinBoxWheelFilter(this);
spinBox->installEventFilter(wheelFilter);
```

## Snapping et fusion de points

### Détection de parent automatique

```cpp
const double snapDistance = 30.0;  // pixels
// Chercher si le point de départ est proche d'une extrémité existante
for (auto* existingSegment : segments) {
    double distToStart = distance(newStart, existingSegment->getStartPoint());
    double distToEnd = distance(newStart, existingSegment->getEndPoint());

    if (distToStart < snapDistance || distToEnd < snapDistance) {
        // Parent trouvé! Snapper exactement sur le point
        snappedStart = (distToStart < distToEnd)
            ? existingSegment->getStartPoint()
            : existingSegment->getEndPoint();
        parentId = existingSegment->getSegmentId();
        break;
    }
}
```

## Valeurs par défaut

### Création de segment

```cpp
lengthSpin->setValue(isEdit ? segment.length : 0.0);      // 0m par défaut
heightSpin->setValue(isEdit ? segment.heightDifference : 0.0); // 0m par défaut
```

### Paramètres réseau

```cpp
supplyPressure = 3.0 bar     // Pression alimentation
requiredPressure = 1.0 bar   // Pression requise
loopLength = 50.0 m          // Longueur bouclage ECS
waterTemp = 60.0°C           // Température eau
ambientTemp = 20.0°C         // Température ambiante
insulation = 19.0 mm         // Épaisseur isolation
```

## Style et UX

### Palette de couleurs Tailwind-inspired

```cpp
// Couleurs principales
#3b82f6  // Bleu primaire (boutons, segments OK)
#e74c3c  // Rouge (alertes, vitesse hors limites)
#3498db  // Bleu segment
#95a5a6  // Gris (non calculé)
#1e293b  // Texte foncé
#f8fafc  // Fond clair
```

### Indicateurs visuels

- **Segment principal**: Cercles rouges plus grands aux extrémités
- **Segment enfant**: Cercles bleus normaux
- **Vitesse OK**: Texte vert foncé (#006600)
- **Vitesse hors limites**: Texte rouge (#cc0000)
- **Non calculé**: Texte gris (#666666)

## Structures de données

### NetworkSegment

```cpp
struct NetworkSegment {
    std::string id;              // ID unique "seg_1", "seg_2"...
    std::string name;            // Nom affiché "Tronçon 1"
    std::string parentId;        // ID du parent ou "" si racine
    double length;               // Longueur en mètres
    double heightDifference;     // Dénivelé en mètres
    std::vector<Fixture> fixtures; // Appareils sur ce segment
    CalculationResult result;    // Résultats du calcul
    double inletPressure;        // Pression entrée
    double outletPressure;       // Pression sortie
};
```

### CalculationResult

```cpp
struct CalculationResult {
    int nominalDiameter;         // DN (10, 12, 16, 20, 25, 32...)
    double actualDiameter;       // Diamètre intérieur réel en mm
    double flowRate;             // Débit en L/min
    double velocity;             // Vitesse en m/s
    double pressureDrop;         // Perte de charge en mCE
    bool hasReturn;              // Retour de bouclage?
    int returnNominalDiameter;   // DN retour
    double returnFlowRate;       // Débit retour L/min
};
```

## Débogage

### Points de vérification

1. **Crash lors modification segment:**
   - Vérifier que `getSegmentId()` est utilisé (pas `getSegmentData()`)
   - Vérifier que `findSegmentById()` est appelé avant `updateDisplay()`

2. **Calcul échoue:**
   - Vérifier messages d'erreur exacts (pas de texte générique)
   - Vérifier hiérarchie parent-enfant dans la validation
   - Vérifier que longueur > 0 pour tous les segments

3. **Affichage incorrect:**
   - Vérifier que `updateDisplay(segmentData)` reçoit bien un pointeur non-null
   - Vérifier que les lambdas de lookup sont passés correctement
   - Vérifier que les résultats sont copiés par ID après calcul

### Logs utiles à ajouter

```cpp
// Debug lookup
qDebug() << "Looking up segment ID:" << QString::fromStdString(id);
NetworkSegment* seg = findSegmentById(id);
qDebug() << "Found segment:" << (seg ? "YES" : "NO");

// Debug calcul
qDebug() << "Calculating" << networkSegments.size() << "segments";
for (const auto& seg : networkSegments) {
    qDebug() << "Segment" << QString::fromStdString(seg.id)
             << "parent:" << QString::fromStdString(seg.parentId)
             << "length:" << seg.length;
}
```

## Tests recommandés

### Test 1: Modification segments multiples
```
1. Créer 3 segments connectés (seg1 → seg2 → seg3)
2. Modifier seg1 (le plus ancien)
3. Modifier seg2 (intermédiaire)
4. Modifier seg3 (le plus récent)
→ Aucun crash attendu
```

### Test 2: Calcul avec hiérarchie valide
```
1. Créer seg1 (racine, pas de parent)
2. Créer seg2 (parent: seg1)
3. Créer seg3 (parent: seg2)
4. Ajouter appareils
5. Calculer
→ Résultats affichés correctement
```

### Test 3: Calcul avec hiérarchie invalide
```
1. Créer seg1
2. Créer seg2 (parent: seg1)
3. Manuellement modifier seg1 pour avoir parent: seg2 (créer boucle)
4. Calculer
→ Message "Boucle circulaire détectée"
```

### Test 4: Valeurs par défaut
```
1. Créer nouveau segment
→ Longueur = 0.0m, Hauteur = 0.0m
```

## Conventions de code

### Nommage

- Variables membres: `camelCase` (ex: `networkSegments`, `currentSelectedSegment`)
- Méthodes: `camelCase` (ex: `findSegmentById`, `updateDisplay`)
- Classes: `PascalCase` (ex: `GraphicPipeSegment`, `HydraulicSchemaView`)
- Constantes: `camelCase` avec const (ex: `const double snapDistance = 30.0`)

### Commentaires en français

```cpp
// Commentaires de code en français
// Messages utilisateur en français
// Messages d'erreur en français
```

### Style Qt

```cpp
// Connexions signals/slots avec nouvelle syntaxe
connect(button, &QPushButton::clicked, this, &ClassName::onButtonClicked);

// Layouts avec setLayout
QVBoxLayout* layout = new QVBoxLayout();
widget->setLayout(layout);

// Ownership Qt: parent possède enfants
QWidget* child = new QWidget(parent);  // parent delete automatiquement
```

## Historique des bugs critiques

### 2024-12: Crash std::length_error (RÉSOLU)
- **Symptôme**: Crash lors modification anciens segments
- **Cause**: Dangling pointers après réallocation vector
- **Solution**: Stockage par ID + lookup dynamique

### 2024-12: Calcul multisegments bad_alloc (RÉSOLU)
- **Symptôme**: Crash avec 2+ segments mais OK avec 1 seul
- **Cause**: Boucles infinies dans hiérarchie parent-enfant invalide
- **Solution**: Validation hiérarchie avant calcul

### 2024-12: Messages validation masqués (RÉSOLU)
- **Symptôme**: Toujours "vérifiez la longueur" au lieu du vrai message
- **Cause**: Exception handler ajoutait texte générique
- **Solution**: Afficher `e.what()` directement sans ajout

## TODO / Améliorations futures

- [ ] Sauvegarder/charger schémas en JSON
- [ ] Undo/Redo pour modifications
- [ ] Export schéma en image (PNG/SVG)
- [ ] Zoom molette + Pan drag
- [ ] Copier/coller segments
- [ ] Import bibliothèque d'appareils personnalisés
- [ ] Calcul incertitudes et tolérances
- [ ] Mode "simulation" avec différents scénarios de débit

## Ressources

- Documentation Qt Graphics View: https://doc.qt.io/qt-6/graphicsview.html
- Normes DTU 60.11: Règles de calcul installations plomberie sanitaire
- Formule Darcy-Weisbach pour pertes de charge

---

*Dernière mise à jour: Décembre 2024*
*Version module: 2.0 (après refonte pointeurs)*
