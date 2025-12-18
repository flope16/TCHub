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
    Sink,                // Évier
    Shower,              // Douche
    Bathtub,             // Baignoire
    WC,                  // WC
    Bidet,               // Bidet
    WashingMachine,      // Lave-linge
    Dishwasher,          // Lave-vaisselle
    UrinalFlush,         // Urinoir à chasse
    UrinalContinuous     // Urinoir à écoulement continu
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
    double returnTemperature;    // Température de l'eau de retour en °C

    // Températures pour ECS (avec et sans bouclage)
    double inletTemperature;     // Température entrée segment en °C
    double outletTemperature;    // Température sortie segment en °C (après pertes)

    std::string recommendation; // Recommandation

    PipeSegmentResult()
        : flowRate(0), velocity(0), pressureDrop(0)
        , nominalDiameter(0), actualDiameter(0)
        , hasReturn(false), returnFlowRate(0), returnVelocity(0)
        , returnNominalDiameter(0), returnActualDiameter(0)
        , heatLoss(0), returnTemperature(0)
        , inletTemperature(0), outletTemperature(0)
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

    // Résultats de calcul
    PipeSegmentResult result;
    double inletPressure;        // Pression en entrée (bar) - calculée
    double outletPressure;       // Pression en sortie (bar) - calculée

    NetworkSegment(const std::string& segId = "", const std::string& segName = "Segment principal")
        : id(segId), name(segName), parentId("")
        , length(0.0), heightDifference(0.0)
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

    // Calcul de la perte de charge singulière (estimée à 20% des pertes linéaires)
    double calculateSingularPressureDrop(double linearDrop);

    // Calcul des pertes thermiques pour le bouclage ECS
    double calculateHeatLoss(double length, double diameter, double insulation,
                            double waterTemp, double ambientTemp);

    // Diamètres nominaux disponibles par matériau
    std::vector<int> getAvailableDiameters(PipeMaterial material);
};

} // namespace HydraulicCalc
