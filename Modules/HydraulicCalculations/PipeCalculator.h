#pragma once

#include <string>
#include <vector>
#include <cmath>

namespace HydraulicCalc {

// Types de réseau
enum class NetworkType {
    ColdWater,           // Eau froide
    HotWater,            // Eau chaude sans bouclage
    HotWaterWithLoop     // Eau chaude avec bouclage
};

// Types d'appareil sanitaire
enum class FixtureType {
    WashBasin,           // Lavabo
    WashBasinCollective, // Lavabo collectif (par jet)
    Sink,                // Évier
    Shower,              // Douche
    Bathtub,             // Baignoire
    WC,                  // WC avec réservoir de chasse
    WCFlushValve,        // WC avec robinet de chasse
    Bidet,               // Bidet
    WashingMachine,      // Lave-linge
    Dishwasher,          // Lave-vaisselle
    UrinalFlush,         // Urinoir avec robinet individuel
    UrinalSiphonic,      // Urinoir à action siphonique
    HandWashBasin,       // Lave-mains
    UtilitySink,         // Bac à laver
    WaterOutlet12,       // Poste d'eau robinet 1/2"
    WaterOutlet34        // Poste d'eau robinet 3/4"
};

// Matériau des tuyaux
enum class PipeMaterial {
    Copper,              // Cuivre
    PEX,                 // PER (polyéthylène réticulé)
    Multilayer,          // Multicouche
    Steel                // Acier galvanisé
};

// Structure pour un appareil sanitaire
struct Fixture {
    FixtureType type;
    int quantity;
    double flowRate;     // Débit unitaire en L/min
    double coefficient;  // Coefficient de simultanéité

    Fixture(FixtureType t, int qty)
        : type(t), quantity(qty), flowRate(0.0), coefficient(1.0) {
        initializeFlowRate();
    }

private:
    void initializeFlowRate();
};

// Détails de calcul intermédiaires pour le débogage (PDF détaillé)
struct CalculationDetails {
    // Calcul du débit
    double totalFixtureFlowRate;   // Somme des débits des appareils (L/min)
    int totalFixtures;              // Nombre total d'appareils
    double simultaneityCoeff;       // Coefficient de simultanéité appliqué

    // Calcul de la vitesse
    double crossSection;            // Section de passage (m²)

    // Calcul de la perte de charge
    double reynolds;                // Nombre de Reynolds
    bool isLaminar;                 // Écoulement laminaire (Re < 2300)
    double lambda;                  // Coefficient de friction (sans dimension)
    double roughness;               // Rugosité absolue du matériau (mm)
    double relativeRoughness;       // Rugosité relative (ε/D)
    double linearPressureDrop;      // Perte de charge linéaire (mCE)
    double singularPressureDrop;    // Perte de charge singulière (mCE)
    double heightPressureDrop;      // Perte/gain dû à la hauteur (mCE)

    // Calcul des pertes thermiques (ECS)
    double r1;                      // Rayon intérieur (m)
    double r2;                      // Rayon extérieur avec isolation (m)
    double thermalResistanceInsul;  // Résistance thermique isolation (K/W par m)
    double thermalResistanceExt;    // Résistance thermique externe (K/W par m)
    double heatLossPerMeter;        // Perte thermique par mètre (W/m)
    double temperatureDrop;         // Chute de température (°C)

    CalculationDetails()
        : totalFixtureFlowRate(0), totalFixtures(0), simultaneityCoeff(0)
        , crossSection(0), reynolds(0), isLaminar(false), lambda(0)
        , roughness(0), relativeRoughness(0), linearPressureDrop(0)
        , singularPressureDrop(0), heightPressureDrop(0)
        , r1(0), r2(0), thermalResistanceInsul(0), thermalResistanceExt(0)
        , heatLossPerMeter(0), temperatureDrop(0)
    {}
};

// Résultat de calcul pour un tronçon
struct PipeSegmentResult {
    double flowRate;         // Débit en L/min
    double velocity;         // Vitesse en m/s
    double pressureDrop;     // Perte de charge en mCE (mètres de colonne d'eau)
    int nominalDiameter;     // Diamètre nominal en mm
    double actualDiameter;   // Diamètre intérieur réel en mm

    // Pour ECS avec bouclage uniquement
    bool hasReturn;          // Indique si un retour de bouclage existe
    double returnFlowRate;   // Débit de retour en L/min
    double returnVelocity;   // Vitesse de retour en m/s
    int returnNominalDiameter;   // Diamètre nominal retour en mm
    double returnActualDiameter; // Diamètre intérieur réel retour en mm
    double heatLoss;         // Pertes thermiques en W
    double returnTemperature;    // Température de l'eau de retour en °C (OBSOLÈTE - utiliser returnOutletTemperature)

    // Températures pour ECS (ALLER : parent → enfant)
    double inletTemperature;     // Température entrée segment aller en °C
    double outletTemperature;    // Température sortie segment aller en °C (après pertes)

    // Températures pour retour bouclage (RETOUR : enfant → parent)
    double returnInletTemperature;   // Température entrée retour en °C (vient des enfants)
    double returnOutletTemperature;  // Température sortie retour en °C (après pertes dans le retour)

    std::string recommendation; // Recommandation

    // Détails de calcul pour le débogage
    CalculationDetails details;

    PipeSegmentResult()
        : flowRate(0), velocity(0), pressureDrop(0)
        , nominalDiameter(0), actualDiameter(0)
        , hasReturn(false), returnFlowRate(0), returnVelocity(0)
        , returnNominalDiameter(0), returnActualDiameter(0)
        , heatLoss(0), returnTemperature(0)
        , inletTemperature(0), outletTemperature(0)
        , returnInletTemperature(0), returnOutletTemperature(0)
    {}
};

// Paramètres de calcul (pour un segment unique)
struct CalculationParameters {
    NetworkType networkType;
    PipeMaterial material;
    double length;               // Longueur du tronçon en m
    double heightDifference;     // Différence de hauteur en m (négatif si descendant)
    double supplyPressure;       // Pression d'alimentation en bar
    double requiredPressure;     // Pression minimale requise à l'appareil en bar
    std::vector<Fixture> fixtures; // Liste des appareils desservis
    int minDiameter;             // Diamètre nominal minimal requis (DN min des enfants)
    double overrideFlowRate;     // Débit forcé (pour segments parents) - 0 = calculer depuis fixtures

    // Paramètres spécifiques pour ECS avec bouclage
    double loopLength;           // Longueur totale de la boucle en m
    double ambientTemperature;   // Température ambiante en °C
    double waterTemperature;     // Température de l'eau en °C (température d'entrée du segment)
    double insulationThickness;  // Épaisseur d'isolation en mm

    CalculationParameters()
        : networkType(NetworkType::ColdWater)
        , material(PipeMaterial::Copper)
        , length(0.0)
        , heightDifference(0.0)
        , supplyPressure(3.0)
        , requiredPressure(1.0)
        , minDiameter(0)
        , overrideFlowRate(0.0)
        , loopLength(0.0)
        , ambientTemperature(20.0)
        , waterTemperature(60.0)
        , insulationThickness(13.0)
    {}
};

// Segment de réseau pour calcul multi-tronçons
struct NetworkSegment {
    std::string id;              // Identifiant unique du segment
    std::string name;            // Nom descriptif (ex: "Branche principale", "Cuisine")
    std::string parentId;        // ID du segment parent (vide si segment racine)

    // Paramètres du segment
    double length;               // Longueur en m
    double heightDifference;     // Différence de hauteur en m
    std::vector<Fixture> fixtures; // Appareils desservis par ce segment
    bool hasReturnLine;          // Indique si ce segment possède une ligne de retour ECS (pour bouclage)

    // Résultats de calcul
    PipeSegmentResult result;
    double inletPressure;        // Pression en entrée (bar) - calculée
    double outletPressure;       // Pression en sortie (bar) - calculée

    NetworkSegment(const std::string& segId = "", const std::string& segName = "Segment principal")
        : id(segId), name(segName), parentId("")
        , length(0.0), heightDifference(0.0)
        , hasReturnLine(false)
        , inletPressure(0.0), outletPressure(0.0)
    {}
};

// Paramètres pour calcul multi-segments
struct NetworkCalculationParameters {
    NetworkType networkType;
    PipeMaterial material;
    double supplyPressure;       // Pression d'alimentation initiale en bar
    double requiredPressure;     // Pression minimale requise aux appareils en bar

    // Paramètres spécifiques pour ECS avec bouclage
    double loopLength;           // Longueur totale de la boucle en m
    double ambientTemperature;   // Température ambiante en °C
    double waterTemperature;     // Température de l'eau en °C
    double insulationThickness;  // Épaisseur d'isolation en mm

    std::vector<NetworkSegment> segments; // Liste de tous les segments du réseau

    NetworkCalculationParameters()
        : networkType(NetworkType::ColdWater)
        , material(PipeMaterial::Copper)
        , supplyPressure(3.0)
        , requiredPressure(1.0)
        , loopLength(0.0)
        , ambientTemperature(20.0)
        , waterTemperature(60.0)
        , insulationThickness(13.0)
    {}
};

// Classe principale de calcul
class PipeCalculator {
public:
    PipeCalculator();
    ~PipeCalculator();

    // Calcul du dimensionnement d'un segment unique
    PipeSegmentResult calculate(const CalculationParameters& params);

    // Calcul du dimensionnement multi-segments
    void calculateNetwork(NetworkCalculationParameters& networkParams);

    // Méthodes utilitaires
    static double getSimultaneityCoefficient(int numberOfFixtures);
    static double calculateFlowRate(const std::vector<Fixture>& fixtures);
    static std::string getFixtureName(FixtureType type);
    static std::string getMaterialName(PipeMaterial material);
    static std::string getNetworkTypeName(NetworkType type);

    // Sélection du diamètre retour avec contraintes de vitesse
    struct ReturnDiameterResult {
        int nominalDiameter;
        double actualDiameter;
        double velocity;
        double adjustedFlowRate;  // Débit ajusté si nécessaire pour respecter vmin
        bool flowRateAdjusted;    // true si le débit a été augmenté
    };
    ReturnDiameterResult selectReturnDiameter(double thermalFlowRate, PipeMaterial material,
                                              double minVelocity = 0.2, double maxVelocity = 0.5);

private:
    // Calculs internes
    double calculatePressureDrop(double flowRate, double length, double diameter,
                                 PipeMaterial material, NetworkType networkType);
    double calculateVelocity(double flowRate, double diameter);
    int selectOptimalDiameter(double flowRate, PipeMaterial material,
                              double maxVelocity = 2.0, int minDiameter = 0);
    double getInternalDiameter(int nominalDiameter, PipeMaterial material);
    double getRoughness(PipeMaterial material);

    // Calcul de la perte de charge linéaire (formule de Colebrook-White simplifiée)
    double calculateLinearPressureDrop(double flowRate, double diameter,
                                       double roughness, double length);

    // Version avec détails pour le PDF
    double calculateLinearPressureDropWithDetails(double flowRate, double diameter,
                                                   double roughness, double length,
                                                   CalculationDetails& details);

    // Calcul de la perte de charge singulière (estimée à 20% des pertes linéaires)
    double calculateSingularPressureDrop(double linearDrop);

    // Calcul des pertes thermiques pour le bouclage ECS
    double calculateHeatLoss(double length, double diameter, double insulation,
                            double waterTemp, double ambientTemp);

    // Version avec détails pour le PDF
    double calculateHeatLossWithDetails(double length, double diameter, double insulation,
                                        double waterTemp, double ambientTemp,
                                        CalculationDetails& details);

    // Version avec détails pour le PDF
    double calculateFlowRateWithDetails(const std::vector<Fixture>& fixtures, CalculationDetails& details);

    // Diamètres nominaux disponibles par matériau
    std::vector<int> getAvailableDiameters(PipeMaterial material);
};

} // namespace HydraulicCalc
