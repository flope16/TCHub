#define _USE_MATH_DEFINES
#include <cmath>
#include "PipeCalculator.h"
#include <algorithm>
#include <functional>

namespace HydraulicCalc {

// Initialisation des débits selon DTU 60.11
void Fixture::initializeFlowRate() {
    switch (type) {
        case FixtureType::WashBasin:
            flowRate = 12.0; // 0.20 L/s = 12 L/min
            break;
        case FixtureType::Sink:
            flowRate = 15.0; // 0.25 L/s = 15 L/min
            break;
        case FixtureType::Shower:
            flowRate = 18.0; // 0.30 L/s = 18 L/min
            break;
        case FixtureType::Bathtub:
            flowRate = 24.0; // 0.40 L/s = 24 L/min
            break;
        case FixtureType::WC:
            flowRate = 7.2; // 0.12 L/s = 7.2 L/min
            break;
        case FixtureType::Bidet:
            flowRate = 12.0; // 0.20 L/s = 12 L/min
            break;
        case FixtureType::WashingMachine:
            flowRate = 18.0; // 0.30 L/s = 18 L/min
            break;
        case FixtureType::Dishwasher:
            flowRate = 15.0; // 0.25 L/s = 15 L/min
            break;
        case FixtureType::UrinalFlush:
            flowRate = 6.0; // 0.10 L/s = 6 L/min
            break;
        case FixtureType::UrinalContinuous:
            flowRate = 2.4; // 0.04 L/s = 2.4 L/min
            break;
    }
}

PipeCalculator::PipeCalculator() {
}

PipeCalculator::~PipeCalculator() {
}

// Coefficient de simultanéité selon la formule couramment utilisée en France
double PipeCalculator::getSimultaneityCoefficient(int numberOfFixtures) {
    if (numberOfFixtures <= 1) return 1.0;
    if (numberOfFixtures == 2) return 0.8;
    if (numberOfFixtures == 3) return 0.7;

    // Formule: K = 1 / sqrt(n-1) pour n > 3
    return 1.0 / std::sqrt(static_cast<double>(numberOfFixtures - 1));
}

double PipeCalculator::calculateFlowRate(const std::vector<Fixture>& fixtures) {
    double totalFlowRate = 0.0;
    int totalFixtures = 0;

    for (const auto& fixture : fixtures) {
        totalFlowRate += fixture.flowRate * fixture.quantity;
        totalFixtures += fixture.quantity;
    }

    // Application du coefficient de simultanéité
    double simultaneityCoeff = getSimultaneityCoefficient(totalFixtures);
    return totalFlowRate * simultaneityCoeff;
}

PipeSegmentResult PipeCalculator::calculate(const CalculationParameters& params) {
    PipeSegmentResult result;

    // Calcul du débit : utiliser overrideFlowRate si défini, sinon calculer depuis fixtures
    if (params.overrideFlowRate > 0.0) {
        result.flowRate = params.overrideFlowRate;
    } else {
        result.flowRate = calculateFlowRate(params.fixtures);
    }

    // Sélection du diamètre optimal (vitesse max 2 m/s pour confort)
    // En tenant compte du DN minimal requis (par ex. DN max des enfants)
    double maxVelocity = (params.networkType == NetworkType::HotWaterWithLoop) ? 1.5 : 2.0;
    result.nominalDiameter = selectOptimalDiameter(result.flowRate, params.material, maxVelocity, params.minDiameter);
    result.actualDiameter = getInternalDiameter(result.nominalDiameter, params.material);

    // Calcul de la vitesse réelle
    result.velocity = calculateVelocity(result.flowRate, result.actualDiameter);

    // Calcul de la perte de charge
    result.pressureDrop = calculatePressureDrop(result.flowRate, params.length,
                                                result.actualDiameter, params.material,
                                                params.networkType);

    // Ajout de la perte de charge due à la différence de hauteur
    result.pressureDrop += params.heightDifference;

    // Vérification de la pression disponible
    double availablePressure = params.supplyPressure - (result.pressureDrop / 10.0);

    // Génération de la recommandation
    result.recommendation = "";

    if (result.velocity > maxVelocity) {
        result.recommendation += "⚠️ Vitesse élevée (" +
            std::to_string(static_cast<int>(result.velocity * 100) / 100.0) +
            " m/s). Risque de bruit. ";
    }

    if (result.velocity < 0.3) {
        result.recommendation += "⚠️ Vitesse faible (" +
            std::to_string(static_cast<int>(result.velocity * 100) / 100.0) +
            " m/s). Risque de stagnation. ";
    }

    if (availablePressure < params.requiredPressure) {
        result.recommendation += "❌ Pression insuffisante (" +
            std::to_string(static_cast<int>(availablePressure * 10) / 10.0) +
            " bar disponibles pour " +
            std::to_string(static_cast<int>(params.requiredPressure * 10) / 10.0) +
            " bar requis). ";
    } else {
        result.recommendation += "✓ Pression suffisante (" +
            std::to_string(static_cast<int>(availablePressure * 10) / 10.0) +
            " bar disponibles). ";
    }

    if (params.networkType == NetworkType::HotWaterWithLoop) {
        result.hasReturn = true;

        // Calcul des pertes thermiques
        result.heatLoss = calculateHeatLoss(params.loopLength, result.actualDiameter,
                                           params.insulationThickness,
                                           params.waterTemperature, params.ambientTemperature);

        // Calcul du débit de retour (bouclage)
        // Formule: Q (L/h) = Pertes (W) / (1.16 * ΔT)
        // ΔT typique = 5°C pour maintenir la température
        double deltaT = 5.0; // Différence de température aller-retour en °C
        double returnFlowRateLh = result.heatLoss / (1.16 * deltaT); // L/h
        result.returnFlowRate = returnFlowRateLh / 60.0; // Conversion en L/min

        // Vitesse minimale recommandée pour éviter stagnation : 0.2-0.3 m/s
        double minReturnVelocity = 0.2;
        double maxReturnVelocity = 0.5;

        // Sélection du diamètre de retour
        result.returnNominalDiameter = selectOptimalDiameter(result.returnFlowRate, params.material, maxReturnVelocity);
        result.returnActualDiameter = getInternalDiameter(result.returnNominalDiameter, params.material);
        result.returnVelocity = calculateVelocity(result.returnFlowRate, result.returnActualDiameter);

        // Ajustement si vitesse trop faible
        while (result.returnVelocity < minReturnVelocity && result.returnNominalDiameter > 10) {
            // Réduire le diamètre pour augmenter la vitesse
            std::vector<int> diameters = getAvailableDiameters(params.material);
            auto it = std::find(diameters.begin(), diameters.end(), result.returnNominalDiameter);
            if (it != diameters.begin()) {
                --it;
                result.returnNominalDiameter = *it;
                result.returnActualDiameter = getInternalDiameter(result.returnNominalDiameter, params.material);
                result.returnVelocity = calculateVelocity(result.returnFlowRate, result.returnActualDiameter);
            } else {
                break;
            }
        }

        // Calcul de la température de retour
        // ΔT = Pertes thermiques / (Débit × Chaleur spécifique × Densité)
        // Chaleur spécifique de l'eau = 4186 J/(kg·K)
        // Densité de l'eau ≈ 1 kg/L
        double flowRateKgPerS = (result.returnFlowRate / 60.0); // L/min → kg/s (densité ≈ 1 kg/L)
        double specificHeat = 4186.0; // J/(kg·K)

        if (flowRateKgPerS > 0) {
            double temperatureDrop = result.heatLoss / (flowRateKgPerS * specificHeat);
            result.returnTemperature = params.waterTemperature - temperatureDrop;
        } else {
            result.returnTemperature = params.waterTemperature - 5.0; // Valeur par défaut si débit nul
        }

        result.recommendation += "Pertes thermiques: " +
            std::to_string(static_cast<int>(result.heatLoss)) + " W. ";
        result.recommendation += "Température retour: " +
            std::to_string(static_cast<int>(result.returnTemperature * 10) / 10.0) + " °C. ";

        if (result.returnVelocity < minReturnVelocity) {
            result.recommendation += "⚠️ Vitesse retour faible (" +
                std::to_string(static_cast<int>(result.returnVelocity * 100) / 100.0) +
                " m/s). Risque de stagnation sur le retour. ";
        }
    }

    if (result.recommendation.empty()) {
        result.recommendation = "✓ Dimensionnement optimal";
    }

    return result;
}

void PipeCalculator::calculateNetwork(NetworkCalculationParameters& networkParams) {
    // Fonction récursive pour collecter tous les appareils d'un segment et tous ses descendants
    std::function<void(const NetworkSegment&, std::vector<Fixture>&)> collectAllFixtures;
    collectAllFixtures = [&](const NetworkSegment& seg, std::vector<Fixture>& fixtures) {
        // Ajouter les fixtures de ce segment
        fixtures.insert(fixtures.end(), seg.fixtures.begin(), seg.fixtures.end());

        // Ajouter récursivement les fixtures de tous les enfants
        for (const auto& otherSegment : networkParams.segments) {
            if (otherSegment.parentId == seg.id) {
                collectAllFixtures(otherSegment, fixtures);
            }
        }
    };

    // Fonction récursive pour calculer un segment BOTTOM-UP (enfants avant parents)
    std::function<void(NetworkSegment&, double)> calculateSegmentRecursive;

    calculateSegmentRecursive = [&](NetworkSegment& segment, double inletPressure) {
        segment.inletPressure = inletPressure;

        // ÉTAPE 1: Calculer d'abord TOUS les enfants (BOTTOM-UP)
        // Collecter les références des enfants directs
        std::vector<NetworkSegment*> children;
        for (auto& childSegment : networkParams.segments) {
            if (childSegment.parentId == segment.id) {
                children.push_back(&childSegment);
                // Calculer récursivement l'enfant AVANT le parent
                // Note: On passe la pression d'entrée actuelle, elle sera mise à jour après
                calculateSegmentRecursive(childSegment, inletPressure);
            }
        }

        // ÉTAPE 2: Déterminer le débit du segment
        double segmentFlowRate = 0.0;

        if (children.empty()) {
            // Segment FEUILLE (sans enfants) : calculer depuis les fixtures avec coeff simultanéité
            segmentFlowRate = calculateFlowRate(segment.fixtures);
        } else {
            // Segment PARENT (avec enfants) : somme des débits des enfants directs
            // + les fixtures directes sur ce segment s'il y en a
            for (const auto* child : children) {
                segmentFlowRate += child->result.flowRate;
            }

            // Ajouter les fixtures directes de ce segment (si présentes)
            if (!segment.fixtures.empty()) {
                segmentFlowRate += calculateFlowRate(segment.fixtures);
            }
        }

        // ÉTAPE 3: Déterminer le DN minimal requis = max des DN de tous les enfants directs
        int minRequiredDiameter = 0;
        for (const auto* child : children) {
            if (child->result.nominalDiameter > minRequiredDiameter) {
                minRequiredDiameter = child->result.nominalDiameter;
            }
        }

        // ÉTAPE 4: Créer les paramètres de calcul pour ce segment
        CalculationParameters params;
        params.networkType = networkParams.networkType;
        params.material = networkParams.material;
        params.length = segment.length;
        params.heightDifference = segment.heightDifference;
        params.supplyPressure = inletPressure;
        params.requiredPressure = networkParams.requiredPressure;
        params.fixtures = segment.fixtures;  // Seulement les fixtures directes
        params.minDiameter = minRequiredDiameter;  // DN minimal = max DN des enfants
        params.overrideFlowRate = (!children.empty()) ? segmentFlowRate : 0.0;  // Forcer débit si parent
        params.loopLength = networkParams.loopLength;
        params.ambientTemperature = networkParams.ambientTemperature;
        params.waterTemperature = networkParams.waterTemperature;
        params.insulationThickness = networkParams.insulationThickness;

        // ÉTAPE 5: Calculer ce segment (le DN sera correct maintenant)
        segment.result = calculate(params);

        // ÉTAPE 6: Calculer la pression de sortie du parent
        segment.outletPressure = segment.inletPressure - (segment.result.pressureDrop / 10.0);

        // ÉTAPE 7: Mettre à jour la pression d'entrée de tous les enfants
        // avec la pression de sortie correcte du parent
        for (auto* child : children) {
            child->inletPressure = segment.outletPressure;
            // Note: Les calculs hydrauliques des enfants sont corrects (DN, vitesse, pertes)
            // Seule la pression d'entrée doit être mise à jour
        }
    };

    // Trouver et calculer tous les segments racines (sans parent)
    for (auto& segment : networkParams.segments) {
        if (segment.parentId.empty()) {
            calculateSegmentRecursive(segment, networkParams.supplyPressure);
        }
    }
}

double PipeCalculator::calculateVelocity(double flowRate, double diameter) {
    // V = Q / A
    // Q en m³/s, A en m²
    double flowRateM3s = flowRate / 60000.0; // L/min -> m³/s
    double area = M_PI * std::pow(diameter / 2000.0, 2); // mm² -> m²
    return flowRateM3s / area;
}

int PipeCalculator::selectOptimalDiameter(double flowRate, PipeMaterial material, double maxVelocity, int minDiameter) {
    std::vector<int> diameters = getAvailableDiameters(material);

    for (int dn : diameters) {
        // Ignorer les diamètres plus petits que le minimum requis
        if (dn < minDiameter) {
            continue;
        }

        double internalDia = getInternalDiameter(dn, material);
        double velocity = calculateVelocity(flowRate, internalDia);

        if (velocity <= maxVelocity) {
            return dn;
        }
    }

    // Si aucun diamètre ne convient, retourner le plus grand disponible
    // ou le minDiameter si tous les diamètres sont trop petits
    if (minDiameter > 0 && diameters.back() < minDiameter) {
        return minDiameter;
    }
    return diameters.back();
}

double PipeCalculator::getInternalDiameter(int nominalDiameter, PipeMaterial material) {
    // Diamètres intérieurs approximatifs en mm
    switch (material) {
        case PipeMaterial::Copper:
            // Pour le cuivre, le DN correspond approximativement au diamètre extérieur
            // Épaisseur de paroi typique: 1mm pour DN10-22, 1.5mm pour DN28+
            if (nominalDiameter <= 22) return nominalDiameter - 2.0;
            else return nominalDiameter - 3.0;

        case PipeMaterial::PEX:
        case PipeMaterial::Multilayer:
            // Pour PER et multicouche, épaisseur de paroi variable
            if (nominalDiameter <= 16) return nominalDiameter - 2.0;
            else if (nominalDiameter <= 20) return nominalDiameter - 2.3;
            else if (nominalDiameter <= 26) return nominalDiameter - 3.0;
            else return nominalDiameter - 3.5;

        case PipeMaterial::Steel:
            // Acier galvanisé: diamètre intérieur nominal
            return nominalDiameter * 0.85; // Approximation
    }
    return nominalDiameter;
}

double PipeCalculator::getRoughness(PipeMaterial material) {
    // Rugosité absolue en mm
    switch (material) {
        case PipeMaterial::Copper:
            return 0.0015; // Cuivre lisse
        case PipeMaterial::PEX:
        case PipeMaterial::Multilayer:
            return 0.007; // PER/Multicouche très lisse
        case PipeMaterial::Steel:
            return 0.15; // Acier galvanisé
    }
    return 0.01;
}

double PipeCalculator::calculatePressureDrop(double flowRate, double length, double diameter,
                                             PipeMaterial material, NetworkType networkType) {
    double roughness = getRoughness(material);
    double linearDrop = calculateLinearPressureDrop(flowRate, diameter, roughness, length);
    double singularDrop = calculateSingularPressureDrop(linearDrop);

    return linearDrop + singularDrop;
}

double PipeCalculator::calculateLinearPressureDrop(double flowRate, double diameter,
                                                   double roughness, double length) {
    // Formule de Darcy-Weisbach simplifiée
    // ΔP = λ * (L/D) * (ρV²/2)
    // Converti en mCE (mètres de colonne d'eau)

    double velocity = calculateVelocity(flowRate, diameter);
    double reynolds = (velocity * diameter / 1000.0) / 0.000001; // Re = VD/ν, ν≈10⁻⁶ m²/s pour l'eau

    // Calcul du coefficient de perte de charge λ (Colebrook-White simplifié)
    double lambda;
    double relativeroughness = roughness / diameter;

    if (reynolds < 2300) {
        // Écoulement laminaire
        lambda = 64.0 / reynolds;
    } else {
        // Écoulement turbulent - Formule de Swamee-Jain (approximation de Colebrook)
        double a = std::pow(roughness / (3.7 * diameter) + 5.74 / std::pow(reynolds, 0.9), 2);
        lambda = 0.25 / std::pow(std::log10(roughness / (3.7 * diameter) + 5.74 / std::pow(reynolds, 0.9)), 2);
    }

    // Perte de charge en mCE
    double pressureDropPa = lambda * (length / (diameter / 1000.0)) * (1000.0 * std::pow(velocity, 2) / 2.0);
    double pressureDropMce = pressureDropPa / (1000.0 * 9.81); // Pa -> mCE

    return pressureDropMce;
}

double PipeCalculator::calculateSingularPressureDrop(double linearDrop) {
    // Estimation des pertes singulières à 20% des pertes linéaires
    // (coudes, vannes, raccords, etc.)
    return linearDrop * 0.20;
}

double PipeCalculator::calculateHeatLoss(double length, double diameter, double insulation,
                                        double waterTemp, double ambientTemp) {
    // Calcul simplifié des pertes thermiques
    // Q = k * A * ΔT
    // où k ≈ 0.3 W/(m·K) pour une isolation de 13mm
    // A = surface externe du tube = π * D * L

    double surfaceArea = M_PI * (diameter / 1000.0 + 2 * insulation / 1000.0) * length;
    double thermalTransmittance = 0.3; // W/(m²·K) - valeur typique pour isolation standard
    double temperatureDiff = waterTemp - ambientTemp;

    return thermalTransmittance * surfaceArea * temperatureDiff;
}

std::vector<int> PipeCalculator::getAvailableDiameters(PipeMaterial material) {
    switch (material) {
        case PipeMaterial::Copper:
            return {10, 12, 14, 16, 18, 22, 28, 35, 42, 54, 64, 76};
        case PipeMaterial::PEX:
        case PipeMaterial::Multilayer:
            return {12, 16, 20, 25, 32, 40, 50, 63};
        case PipeMaterial::Steel:
            return {15, 20, 26, 32, 40, 50, 65, 80, 100};
    }
    return {16, 20, 25, 32, 40, 50};
}

// Fonctions utilitaires pour les noms
std::string PipeCalculator::getFixtureName(FixtureType type) {
    switch (type) {
        case FixtureType::WashBasin: return "Lavabo";
        case FixtureType::Sink: return "Évier";
        case FixtureType::Shower: return "Douche";
        case FixtureType::Bathtub: return "Baignoire";
        case FixtureType::WC: return "WC";
        case FixtureType::Bidet: return "Bidet";
        case FixtureType::WashingMachine: return "Lave-linge";
        case FixtureType::Dishwasher: return "Lave-vaisselle";
        case FixtureType::UrinalFlush: return "Urinoir à chasse";
        case FixtureType::UrinalContinuous: return "Urinoir à écoulement";
    }
    return "Inconnu";
}

std::string PipeCalculator::getMaterialName(PipeMaterial material) {
    switch (material) {
        case PipeMaterial::Copper: return "Cuivre";
        case PipeMaterial::PEX: return "PER";
        case PipeMaterial::Multilayer: return "Multicouche";
        case PipeMaterial::Steel: return "Acier galvanisé";
    }
    return "Inconnu";
}

std::string PipeCalculator::getNetworkTypeName(NetworkType type) {
    switch (type) {
        case NetworkType::ColdWater: return "Eau froide";
        case NetworkType::HotWater: return "Eau chaude sanitaire";
        case NetworkType::HotWaterWithLoop: return "ECS avec bouclage";
    }
    return "Inconnu";
}

} // namespace HydraulicCalc
