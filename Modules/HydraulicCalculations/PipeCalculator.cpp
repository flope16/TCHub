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
    // DTU 60.11 : viser 0,5 à 1,0 m/s en ECS pour limiter le bruit/érosion
    double maxVelocity = (params.networkType == NetworkType::ColdWater) ? 2.0 : 1.0;
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

    // Calcul des températures pour ECS (avec et sans bouclage)
    if (params.networkType == NetworkType::HotWater || params.networkType == NetworkType::HotWaterWithLoop) {
        // Température d'entrée = température fournie en paramètre
        result.inletTemperature = params.waterTemperature;

        // Calcul des pertes thermiques pour ce segment
        result.heatLoss = calculateHeatLoss(params.length, result.actualDiameter,
                                           params.insulationThickness,
                                           params.waterTemperature, params.ambientTemperature);

        // Calcul de la chute de température due aux pertes thermiques
        // ΔT = Pertes (W) / (Débit (L/min) × Chaleur spécifique (J/(kg·K)) × Densité (kg/L) / 60)
        // Chaleur spécifique de l'eau = 4186 J/(kg·K), Densité ≈ 1 kg/L
        if (result.flowRate > 0) {
            double flowRateKgPerS = result.flowRate / 60.0;  // L/min → kg/s (densité ≈ 1 kg/L)
            double specificHeat = 4186.0;  // J/(kg·K)
            double temperatureDrop = result.heatLoss / (flowRateKgPerS * specificHeat);
            result.outletTemperature = result.inletTemperature - temperatureDrop;
        } else {
            // Si débit nul, température reste identique (pas de refroidissement)
            result.outletTemperature = result.inletTemperature;
        }

        // Contrôle DTU 60.11 : ne pas descendre sous 55°C
        const double minHotWaterTemp = 55.0;
        if (result.outletTemperature < minHotWaterTemp) {
            double allowableDrop = result.inletTemperature - minHotWaterTemp;
            if (allowableDrop > 0.1) {
                double minFlowKgPerS = result.heatLoss / (4186.0 * allowableDrop);
                double minFlowLPerMin = minFlowKgPerS * 60.0;  // kg/s ≈ L/s → L/min
                result.recommendation += "⚠️ Débit insuffisant pour maintenir 55°C (" +
                    std::to_string(static_cast<int>(minFlowLPerMin * 10) / 10.0) +
                    " L/min requis). ";
            } else {
                result.recommendation += "⚠️ Température trop basse (<55°C). ";
            }
        }

        // Alerte si ΔT > 1°C / 10 m avec isolant standard
        double deltaT = result.inletTemperature - result.outletTemperature;
        if (params.length > 0.1 && deltaT > (params.length / 10.0)) {
            result.recommendation += "⚠️ ΔT élevé (" +
                std::to_string(static_cast<int>(deltaT * 10) / 10.0) +
                " °C sur " + std::to_string(static_cast<int>(params.length * 10) / 10.0) +
                " m). Vérifier isolant ou débit. ";
        }
    } else {
        // Pour eau froide, pas de calcul de température
        result.inletTemperature = 0.0;
        result.outletTemperature = 0.0;
        result.heatLoss = 0.0;
    }

    // Vérification de la pression disponible
    double availablePressure = params.supplyPressure - (result.pressureDrop / 10.0);

    // Génération de la recommandation
    result.recommendation = "";

    if (result.velocity > maxVelocity) {
        result.recommendation += "⚠️ Vitesse élevée (" +
            std::to_string(static_cast<int>(result.velocity * 100) / 100.0) +
            " m/s). Risque de bruit. ";
    }

    double minVelocityWarning = (params.networkType == NetworkType::ColdWater) ? 0.3 : 0.5;
    if (result.velocity < minVelocityWarning) {
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
        // NOTE: Le débit de retour, DN retour et température retour sont calculés
        // globalement dans calculateNetwork() après avoir calculé tous les segments
        // car le débit de retour doit être identique pour TOUTE la boucle
    }

    if (result.recommendation.empty()) {
        result.recommendation = "✓ Dimensionnement optimal";
    }

    return result;
}

void PipeCalculator::calculateNetwork(NetworkCalculationParameters& networkParams) {
    // Calcul automatique de la longueur de boucle = somme des longueurs de tous les segments
    double totalLoopLength = 0.0;
    for (const auto& segment : networkParams.segments) {
        totalLoopLength += segment.length;
    }
    networkParams.loopLength = totalLoopLength;  // Mise à jour automatique

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

        // ÉTAPE 7: Mettre à jour la pression et température d'entrée de tous les enfants
        // avec les valeurs de sortie correctes du parent
        for (auto* child : children) {
            child->inletPressure = segment.outletPressure;

            // Pour ECS: propager la température de sortie du parent vers l'entrée des enfants
            // NOTE: les enfants ont déjà été calculés, on doit recalculer avec la bonne température
            if (networkParams.networkType == NetworkType::HotWater ||
                networkParams.networkType == NetworkType::HotWaterWithLoop) {

                // Recalculer les pertes thermiques et la température de sortie de l'enfant
                // avec la température d'entrée correcte (= température sortie parent)
                double childInletTemp = segment.result.outletTemperature;
                child->result.inletTemperature = childInletTemp;

                // Recalculer les pertes thermiques avec la bonne température
                child->result.heatLoss = calculateHeatLoss(child->length, child->result.actualDiameter,
                                                          networkParams.insulationThickness,
                                                          childInletTemp, networkParams.ambientTemperature);

                // Recalculer la température de sortie
                if (child->result.flowRate > 0) {
                    double flowRateKgPerS = child->result.flowRate / 60.0;
                    double specificHeat = 4186.0;
                    double temperatureDrop = child->result.heatLoss / (flowRateKgPerS * specificHeat);
                    child->result.outletTemperature = childInletTemp - temperatureDrop;
                } else {
                    child->result.outletTemperature = childInletTemp;
                }
            }
        }
    };

    // Trouver et calculer tous les segments racines (sans parent)
    for (auto& segment : networkParams.segments) {
        if (segment.parentId.empty()) {
            calculateSegmentRecursive(segment, networkParams.supplyPressure);
        }
    }

    // PASSE 2: Calcul du retour bouclage (si applicable)
    if (networkParams.networkType == NetworkType::HotWaterWithLoop) {
        // Dans un système de bouclage avec sous-boucles :
        // - Chaque sous-boucle calcule son propre débit de retour basé sur ses pertes
        // - Au parent : débits s'additionnent, températures se mélangent (pondérées par débits)

        // ΔT acceptable sur une boucle
        const double deltaT = 5.0;  // °C

        // Vitesses min/max recommandées pour le retour
        const double minReturnVelocity = 0.5;  // m/s (DTU : viser 0,5 à 1,0 m/s)
        const double maxReturnVelocity = 1.0;  // m/s

        // PASSE 3: Calcul récursif bottom-up des débits de retour et températures
        // Tout se fait en une passe : enfant → parent

        std::function<void(NetworkSegment&)> calculateRetourRecursive;
        calculateRetourRecursive = [&](NetworkSegment& segment) {
            // D'abord, calculer récursivement tous les enfants
            std::vector<NetworkSegment*> children;
            for (auto& child : networkParams.segments) {
                if (child.parentId == segment.id) {
                    children.push_back(&child);
                    calculateRetourRecursive(child);
                }
            }

            // ÉTAPE 1 : Calculer le débit de retour du segment
            if (children.empty()) {
                // Segment FEUILLE : calcul du débit basé sur les pertes locales
                // Q_retour (L/h) = Pertes (W) / (1.16 × ΔT)
                double returnFlowRateLh = segment.result.heatLoss / (1.16 * deltaT);
                segment.result.returnFlowRate = returnFlowRateLh / 60.0;  // L/h → L/min
            } else {
                // Segment PARENT : somme des débits de retour des enfants
                double totalReturnFlow = 0.0;
                for (const auto* child : children) {
                    totalReturnFlow += child->result.returnFlowRate;
                }
                segment.result.returnFlowRate = totalReturnFlow;
            }

            // ÉTAPE 2 : Calculer le DN retour basé sur le débit local
            segment.result.hasReturn = true;
            segment.result.returnNominalDiameter = selectOptimalDiameter(
                segment.result.returnFlowRate, networkParams.material, maxReturnVelocity);
            segment.result.returnActualDiameter = getInternalDiameter(
                segment.result.returnNominalDiameter, networkParams.material);
            segment.result.returnVelocity = calculateVelocity(
                segment.result.returnFlowRate, segment.result.returnActualDiameter);

            // Ajustement si vitesse trop faible (réduire DN)
            while (segment.result.returnVelocity < minReturnVelocity &&
                   segment.result.returnNominalDiameter > 10) {
                std::vector<int> diameters = getAvailableDiameters(networkParams.material);
                auto it = std::find(diameters.begin(), diameters.end(),
                                   segment.result.returnNominalDiameter);
                if (it != diameters.begin()) {
                    --it;
                    segment.result.returnNominalDiameter = *it;
                    segment.result.returnActualDiameter = getInternalDiameter(
                        segment.result.returnNominalDiameter, networkParams.material);
                    segment.result.returnVelocity = calculateVelocity(
                        segment.result.returnFlowRate, segment.result.returnActualDiameter);
                } else {
                    break;
                }
            }

            // ÉTAPE 3 : Calculer la température d'entrée retour (returnInletTemperature)
            if (children.empty()) {
                // Segment FEUILLE : l'eau passe de l'aller au retour
                segment.result.returnInletTemperature = segment.result.outletTemperature;
            } else {
                // Segment PARENT : MÉLANGE PONDÉRÉ par les débits
                // Formule : T_mélange = (Q₁×T₁ + Q₂×T₂ + ...) / (Q₁ + Q₂ + ...)
                double sumWeightedTemp = 0.0;
                double sumFlow = 0.0;

                for (const auto* child : children) {
                    sumWeightedTemp += child->result.returnFlowRate * child->result.returnOutletTemperature;
                    sumFlow += child->result.returnFlowRate;
                }

                if (sumFlow > 0.0001) {
                    segment.result.returnInletTemperature = sumWeightedTemp / sumFlow;
                } else {
                    // Cas dégénéré : moyenne simple si débits nuls
                    double sumTemp = 0.0;
                    for (const auto* child : children) {
                        sumTemp += child->result.returnOutletTemperature;
                    }
                    segment.result.returnInletTemperature = sumTemp / children.size();
                }
            }

            // ÉTAPE 4 : Calculer les pertes thermiques dans le retour
            double returnHeatLoss = calculateHeatLoss(
                segment.length, segment.result.returnActualDiameter,
                networkParams.insulationThickness,
                segment.result.returnInletTemperature, networkParams.ambientTemperature);

            // ÉTAPE 5 : Calculer la température de sortie retour
            // Formule : ΔT = Pertes (W) / (1160 × Q_retour (m³/h))
            double returnFlowRateM3h = segment.result.returnFlowRate * 60.0 / 1000.0;  // L/min → m³/h
            double temperatureDrop = 0.0;
            if (returnFlowRateM3h > 0.0001) {
                temperatureDrop = returnHeatLoss / (1160.0 * returnFlowRateM3h);
            }

            segment.result.returnOutletTemperature = segment.result.returnInletTemperature - temperatureDrop;

            // Compatibilité avec ancien champ
            segment.result.returnTemperature = segment.result.returnOutletTemperature;
        };

        // Calculer les débits et DN de retour depuis les segments racines
        for (auto& segment : networkParams.segments) {
            if (segment.parentId.empty()) {
                calculateRetourRecursive(segment);
            }
        }

        // PASSE 3A: Recalculer les températures ALLER avec le débit de bouclage
        // Dans un système de bouclage, c'est le débit de RETOUR qui circule à l'aller (circuit fermé)
        // On doit donc recalculer les températures aller avec ce débit, pas avec le débit des fixtures

        std::function<void(NetworkSegment&)> recalculateAllerTemperatures;
        recalculateAllerTemperatures = [&](NetworkSegment& segment) {
            // La température d'entrée a déjà été définie (source ou parent)

            // Recalculer les pertes thermiques ALLER avec le débit de bouclage
            // Le débit de bouclage à l'aller = débit de retour du segment
            double bouclageFlowRateM3h = segment.result.returnFlowRate * 60.0 / 1000.0;  // L/min → m³/h

            segment.result.heatLoss = calculateHeatLoss(
                segment.length, segment.result.actualDiameter,
                networkParams.insulationThickness,
                segment.result.inletTemperature, networkParams.ambientTemperature);

            // Calculer la chute de température ALLER avec le débit de bouclage
            // Formule : ΔT = Pertes (W) / (1160 × Q_bouclage (m³/h))
            double temperatureDrop = 0.0;
            if (bouclageFlowRateM3h > 0.0001) {
                temperatureDrop = segment.result.heatLoss / (1160.0 * bouclageFlowRateM3h);
            }

            segment.result.outletTemperature = segment.result.inletTemperature - temperatureDrop;

            // Propager aux enfants
            for (auto& child : networkParams.segments) {
                if (child.parentId == segment.id) {
                    // Température sortie parent = température entrée enfant
                    child.result.inletTemperature = segment.result.outletTemperature;
                    recalculateAllerTemperatures(child);
                }
            }
        };

        // Initialiser température source et calculer récursivement les températures ALLER
        for (auto& segment : networkParams.segments) {
            if (segment.parentId.empty()) {
                segment.result.inletTemperature = networkParams.waterTemperature;
                recalculateAllerTemperatures(segment);
            }
        }

        // PASSE 3B: Recalculer les températures RETOUR avec les nouvelles températures ALLER
        // Maintenant que les températures aller sont correctes, on recalcule les retours

        std::function<void(NetworkSegment&)> recalculateRetourTemperatures;
        recalculateRetourTemperatures = [&](NetworkSegment& segment) {
            // D'abord, calculer récursivement tous les enfants
            std::vector<NetworkSegment*> children;
            for (auto& child : networkParams.segments) {
                if (child.parentId == segment.id) {
                    children.push_back(&child);
                    recalculateRetourTemperatures(child);
                }
            }

            // Calculer returnInletTemperature du segment actuel
            if (children.empty()) {
                // Segment FEUILLE : l'eau passe de l'aller au retour
                segment.result.returnInletTemperature = segment.result.outletTemperature;
            } else {
                // Segment PARENT : MÉLANGE PONDÉRÉ par les débits
                double sumWeightedTemp = 0.0;
                double sumFlow = 0.0;

                for (const auto* child : children) {
                    sumWeightedTemp += child->result.returnFlowRate * child->result.returnOutletTemperature;
                    sumFlow += child->result.returnFlowRate;
                }

                if (sumFlow > 0.0001) {
                    segment.result.returnInletTemperature = sumWeightedTemp / sumFlow;
                } else {
                    double sumTemp = 0.0;
                    for (const auto* child : children) {
                        sumTemp += child->result.returnOutletTemperature;
                    }
                    segment.result.returnInletTemperature = sumTemp / children.size();
                }
            }

            // Calculer les pertes thermiques dans le retour
            double returnHeatLoss = calculateHeatLoss(
                segment.length, segment.result.returnActualDiameter,
                networkParams.insulationThickness,
                segment.result.returnInletTemperature, networkParams.ambientTemperature);

            // Calculer la température de sortie retour
            double returnFlowRateM3h = segment.result.returnFlowRate * 60.0 / 1000.0;
            double temperatureDrop = 0.0;
            if (returnFlowRateM3h > 0.0001) {
                temperatureDrop = returnHeatLoss / (1160.0 * returnFlowRateM3h);
            }

            segment.result.returnOutletTemperature = segment.result.returnInletTemperature - temperatureDrop;
            segment.result.returnTemperature = segment.result.returnOutletTemperature;
        };

        // Recalculer les températures retour depuis les segments racines
        for (auto& segment : networkParams.segments) {
            if (segment.parentId.empty()) {
                recalculateRetourTemperatures(segment);
            }
        }

        // PASSE 4: Ajuster les débits de retour pour respecter la contrainte de température
        // La température au retour (source) doit être >= T_source - ΔT_max
        double minReturnTemp = networkParams.waterTemperature - deltaT;  // Ex: 60°C - 5°C = 55°C

        // Fonction pour recalculer les températures avec un nouveau débit
        std::function<void(NetworkSegment&, double)> recalculateTemperaturesWithFlow;
        recalculateTemperaturesWithFlow = [&](NetworkSegment& segment, double newFlowRate) {
            // Mettre à jour le débit
            segment.result.returnFlowRate = newFlowRate;

            // Recalculer DN et vitesse avec le nouveau débit
            segment.result.returnNominalDiameter = selectOptimalDiameter(
                newFlowRate, networkParams.material, maxReturnVelocity);
            segment.result.returnActualDiameter = getInternalDiameter(
                segment.result.returnNominalDiameter, networkParams.material);
            segment.result.returnVelocity = calculateVelocity(
                newFlowRate, segment.result.returnActualDiameter);

            // Ajustement si vitesse trop faible (réduire DN)
            while (segment.result.returnVelocity < minReturnVelocity &&
                   segment.result.returnNominalDiameter > 10) {
                std::vector<int> diameters = getAvailableDiameters(networkParams.material);
                auto it = std::find(diameters.begin(), diameters.end(),
                                   segment.result.returnNominalDiameter);
                if (it != diameters.begin()) {
                    --it;
                    segment.result.returnNominalDiameter = *it;
                    segment.result.returnActualDiameter = getInternalDiameter(
                        segment.result.returnNominalDiameter, networkParams.material);
                    segment.result.returnVelocity = calculateVelocity(
                        newFlowRate, segment.result.returnActualDiameter);
                } else {
                    break;
                }
            }

            // Ajustement si vitesse trop élevée (augmenter DN)
            while (segment.result.returnVelocity > maxReturnVelocity) {
                std::vector<int> diameters = getAvailableDiameters(networkParams.material);
                auto it = std::find(diameters.begin(), diameters.end(),
                                   segment.result.returnNominalDiameter);
                if (it != diameters.end() && (it + 1) != diameters.end()) {
                    ++it;
                    segment.result.returnNominalDiameter = *it;
                    segment.result.returnActualDiameter = getInternalDiameter(
                        segment.result.returnNominalDiameter, networkParams.material);
                    segment.result.returnVelocity = calculateVelocity(
                        newFlowRate, segment.result.returnActualDiameter);
                } else {
                    break;
                }
            }

            // Recalculer les pertes thermiques retour avec le nouveau DN
            double returnHeatLoss = calculateHeatLoss(
                segment.length, segment.result.returnActualDiameter,
                networkParams.insulationThickness,
                segment.result.returnInletTemperature, networkParams.ambientTemperature);

            // Recalculer température de sortie retour
            double returnFlowRateM3h = newFlowRate * 60.0 / 1000.0;
            double temperatureDrop = 0.0;
            if (returnFlowRateM3h > 0.0001) {
                temperatureDrop = returnHeatLoss / (1160.0 * returnFlowRateM3h);
            }

            segment.result.returnOutletTemperature = segment.result.returnInletTemperature - temperatureDrop;
            segment.result.returnTemperature = segment.result.returnOutletTemperature;

            // Propager le nouveau débit aux enfants proportionnellement
            std::vector<NetworkSegment*> children;
            double oldTotalChildFlow = 0.0;
            for (auto& child : networkParams.segments) {
                if (child.parentId == segment.id) {
                    children.push_back(&child);
                    oldTotalChildFlow += child.result.returnFlowRate;
                }
            }

            if (!children.empty() && oldTotalChildFlow > 0.0001) {
                // Distribuer le nouveau débit proportionnellement aux enfants
                for (auto* child : children) {
                    double ratio = child->result.returnFlowRate / oldTotalChildFlow;
                    double childNewFlow = newFlowRate * ratio;
                    recalculateTemperaturesWithFlow(*child, childNewFlow);
                }

                // Recalculer le mélange pondéré avec les nouveaux débits
                double sumWeightedTemp = 0.0;
                double sumFlow = 0.0;
                for (const auto* child : children) {
                    sumWeightedTemp += child->result.returnFlowRate * child->result.returnOutletTemperature;
                    sumFlow += child->result.returnFlowRate;
                }
                if (sumFlow > 0.0001) {
                    segment.result.returnInletTemperature = sumWeightedTemp / sumFlow;
                }

                // Recalculer la sortie retour du parent avec la nouvelle inlet
                returnHeatLoss = calculateHeatLoss(
                    segment.length, segment.result.returnActualDiameter,
                    networkParams.insulationThickness,
                    segment.result.returnInletTemperature, networkParams.ambientTemperature);

                temperatureDrop = 0.0;
                if (returnFlowRateM3h > 0.0001) {
                    temperatureDrop = returnHeatLoss / (1160.0 * returnFlowRateM3h);
                }
                segment.result.returnOutletTemperature = segment.result.returnInletTemperature - temperatureDrop;
                segment.result.returnTemperature = segment.result.returnOutletTemperature;
            }

            // Après avoir propagé les nouveaux débits, recalculer toutes les températures
            // ALLER (top-down) puis RETOUR (bottom-up) depuis ce segment

            // Recalculer températures ALLER avec nouveau débit
            recalculateAllerTemperatures(segment);

            // Recalculer températures RETOUR avec nouvelles températures aller
            recalculateRetourTemperatures(segment);
        };

        // Appliquer la contrainte de température 55°C sur chaque segment de bouclage
        std::function<void(NetworkSegment&)> enforceMinimumReturnTemperature;
        enforceMinimumReturnTemperature = [&](NetworkSegment& segment) {
            double returnHeatLoss = calculateHeatLoss(
                segment.length, segment.result.returnActualDiameter,
                networkParams.insulationThickness,
                segment.result.returnInletTemperature, networkParams.ambientTemperature);

            double allowableDrop = segment.result.returnInletTemperature - minReturnTemp;
            if (allowableDrop > 0.05) {
                double minFlowM3h = returnHeatLoss / (1160.0 * allowableDrop);
                double minFlowLmin = minFlowM3h * 1000.0 / 60.0;

                if (segment.result.returnFlowRate + 1e-6 < minFlowLmin) {
                    recalculateTemperaturesWithFlow(segment, minFlowLmin);
                }
            }

            for (auto& child : networkParams.segments) {
                if (child.parentId == segment.id) {
                    enforceMinimumReturnTemperature(child);
                }
            }
        };

        for (auto& segment : networkParams.segments) {
            if (segment.parentId.empty()) {
                enforceMinimumReturnTemperature(segment);
            }
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
    // Modèle simplifié issu des notes DTU 60.11 :
    // q = U × π × D_ext × (T_eau - T_amb)
    // U dépend de l'épaisseur d'isolant (≈0,5 à 1,0 W/m².K pour 20–30 mm)

    // Diamètre extérieur isolé (m)
    double externalDiameterM = (diameter + 2.0 * insulation) / 1000.0;

    // Ajustement empirique de U en fonction de l'épaisseur d'isolant
    double effectiveThickness = std::max(insulation, 5.0); // éviter division par zéro
    double U = 0.8 * (25.0 / effectiveThickness); // 0,8 W/m².K à 25 mm
    U = std::clamp(U, 0.4, 1.6); // bornes cohérentes isolant courant

    double temperatureDiff = waterTemp - ambientTemp;
    double qLinear = U * M_PI * externalDiameterM * temperatureDiff; // W/m

    return qLinear * length;
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
