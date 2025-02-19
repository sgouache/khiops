// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWClassStats.h"
#include "KWDataGrid.h"
#include "KWSortableIndex.h"
#include "KWAttributeStats.h"
#include "KWQuantileBuilder.h"
#include "KWFrequencyVector.h"

/////////////////////////////////////////////////////////////////////////////////////
// Gestion de grilles de donnees
// Copies et transferts partiels ou total du contenu d'une grille source
//  pour alimenter une grille cible.
// La grille source est specifiee une fois pour toute.
// Les grilles sources et cible peut etre d'une sous-classe quelconque de KWDataGrid.
// Par contre, seules les parties geree au niveau de KWDataGrid sont explicitement
// gerees (donc, le contenu de la grille en attributs, parties et cellules).
class KWDataGridManager : public Object
{
public:
	// Constructeur
	KWDataGridManager();
	~KWDataGridManager();

	// Copie du contenu d'une grille source vers une grille cible
	void CopyDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* targetDataGrid) const;

	// Copie de la partie informative du contenu d'une grille source vers une grille cible
	// La partie informative est constituee des attributs non reduits a une seule partie
	void CopyInformativeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* targetDataGrid) const;

	// Grille de donnees source
	// Parametrage necessaire pour toutes les operations d'exports total ou partiel d'une grille
	// Memoire: appartient a l'appelant
	void SetSourceDataGrid(const KWDataGrid* dataGrid);
	const KWDataGrid* GetSourceDataGrid() const;

	///////////////////////////////////////////////////////////////////////////
	// Service de transfert du contenu de la grille source vers la grille cible
	// Les methodes prennent en argument un grille initialement vide
	// Pour toutes les methodes de la classe, l'ordre des attributs exportes est
	// compatible avec celui des attributs initaux

	// Export total (attribut, parties et cellules)
	void ExportDataGrid(KWDataGrid* targetDataGrid) const;

	// Export d'une grille terminale, avec attributs reduits a une seule partie
	void ExportTerminalDataGrid(KWDataGrid* targetDataGrid) const;

	// Export des attributs uniquement (plus les specifications des classes cibles)
	void ExportAttributes(KWDataGrid* targetDataGrid) const;

	// Export d'un unique attribut (plus les specifications des classes cibles)
	void ExportOneAttribute(KWDataGrid* targetDataGrid, const ALString& sAttributeName) const;

	// Export des attributs informatifs uniquement (non reduits a une seule partie)
	void ExportInformativeAttributes(KWDataGrid* targetDataGrid) const;

	// Export des parties uniquement (les attributs de la grille cible sont deja exportes)
	// Les attributs cibles peuvent n'etre qu'un sous-ensemble des attributs sources
	void ExportParts(KWDataGrid* targetDataGrid) const;

	// Export des parties pour un attribut donne (devant exister dans la grille source et
	// dans la grille cible sans ses parties)
	void ExportPartsForAttribute(KWDataGrid* targetDataGrid, const ALString& sAttributeName) const;

	// Export des cellules uniquement (les attributs et parties de la grille cible sont deja exportes)
	// Les attributs cibles peuvent n'etre qu'un sous-ensemble des attributs sources
	// Les parties cibles peuvent former une partition quelconque des parties sources
	void ExportCells(KWDataGrid* targetDataGrid) const;

	///////////////////////////////////////////////////////////////////////////
	// Construction d'une grille de donnees cible en partitionnant aleatoirement
	// la grille de donnees source
	// Les methodes suivantes travaillent uniquement sur la structure des grilles
	// cibles, qui sont vides de cellules avant et apres l'appel de chaque methode.
	// Les methodes ExportRandom... creent une nouvelle grille par generation aleatoire
	// d'attributs et parties.
	// Les methodes AddRandom... contraignent la generation aleatoire par une
	// grille comportant les attributs et parties obligatoires

	// Export d'un sous-ensemble aleatoire des attributs (plus les specifications des classes cibles)
	void ExportRandomAttributes(KWDataGrid* targetDataGrid, int nAttributeNumber) const;

	// Export d'une partition aleatoire des parties pour chaque attribut cible.
	// Le nombre de parties effectif peut etre inferieur au nombre specifie, s'il n'y
	// a pas assez de valeurs sources. Le choix de la partition est uniforme
	// (partition des rangs pour les attributs numeriques, des valeurs pour les symboliques).
	void ExportRandomParts(KWDataGrid* targetDataGrid, int nMeanAttributePartNumber) const;

	// Export d'une partition aleatoire des parties pour un attribut donne
	void ExportRandomAttributeParts(KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute,
					KWDGAttribute* targetAttribute, int nPartNumber) const;

	// Export d'un sous-ensemble aleatoire des attributs (plus les specifications des classes cibles)
	// en partant d'un ensemble d'attributs obligatoires.
	void AddRandomAttributes(KWDataGrid* targetDataGrid, const KWDataGrid* mandatoryDataGrid,
				 int nRequestedAttributeNumber) const;

	// Export d'une partition aleatoire des parties pour chaque attribut cible
	// en partant de partitions aleatoires pour un sous-ensemble d'attributs.
	// Les nouvelles parties sont obtenues en sur-partitionnant les partitions existantes,
	// pour atteindre les nombres de parties a ajouter demandees par type d'attribut.
	// Le nombre de partie a ajouter demande est aleatoire, avec au moins le pourcentage minimum demande.
	// Le nombre de parties reellement ajoutees peut etre inferieur a celui demande, s'il n'y
	// a pas assez de valeurs disponibles pour scinder des parties existantes
	void AddRandomParts(KWDataGrid* targetDataGrid, const KWDataGrid* mandatoryDataGrid,
			    int nRequestedContinuousPartNumber, int nRequestedSymbolPartNumber,
			    double dMinPercentageAddedPart) const;

	// Ajout aleatoire de partie dans une partition pour un attribut donne
	void AddRandomAttributeParts(KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute,
				     KWDGAttribute* mandatoryAttribute, KWDGAttribute* targetAttribute,
				     int nRequestedPartNumber) const;

	///////////////////////////////////////////////////////////////////////////
	// Construction d'une grille de donnees cible en granularisant
	// la grille de donnees source

	// Initialisation du dictionnaire des quantiles builders
	// Un quantile builder (Group ou Interval) est initialise par attribut de la grille source.
	// Il est range dans un dictionnaire a partir du nom de l'attribut source
	// Une fois initialises, ces quantile builders sont utilises pour les granularisations
	// En sortie, le vecteur ivMaxPartNumbers contient pour chaque attribut le nombre maximal
	// de parties attendu apres granularisation
	// Pour un attribut numerique, il s'agit du nombre de valeurs distinctes
	// Pour un attribut categoriel,  il s'agit du nombre de parties dont l'effectif
	//  est > 1 + 1 en presence de singletons
	void InitializeQuantileBuildersBeforeGranularization(ObjectDictionary* odQuantilesBuilders,
							     IntVector* ivMaxPartNumbers) const;

	// Export d'une grille granularisee pour une granularite commune a tous ses attributs (attribut, parties et
	// cellules)
	void ExportGranularizedDataGrid(KWDataGrid* targetDataGrid, int nGranularity,
					ObjectDictionary* odQuantilesBuilders) const;

	// Export des parties granularisees (les attributs de la grille cible sont deja exportes)
	void ExportGranularizedParts(KWDataGrid* targetDataGrid, int nGranularity,
				     ObjectDictionary* odQuantilesBuilders) const;

	// Export d'une partition definie par une granularite pour un attribut donne
	// Met a jour le nombre de partiles (nPartileNumber) effectivement obtenue pour cette granularite
	// void ExportGranularizedPartsForAttribute(KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute,
	// KWDGAttribute* targetAttribute, int nGranularity) const;
	void ExportGranularizedPartsForContinuousAttribute(KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute,
							   KWDGAttribute* targetAttribute, int nGranularity,
							   KWQuantileIntervalBuilder* quantileIntervalBuilder) const;
	void ExportGranularizedPartsForSymbolAttribute(KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute,
						       KWDGAttribute* targetAttribute, int nGranularity,
						       KWQuantileGroupBuilder* quantileGroupBuilder) const;

	///////////////////////////////////////////////////////////////////////////
	// Service de recuperation des partitions univariees pour initialiser
	// la grille cible (de facon compatible avec la grille source)

	// Creation de la grille cible a partir d'une partition univariee
	// Ajout de la transmission de la granularite a la grille cible
	void BuildDataGridFromUnivariateStats(KWDataGrid* targetDataGrid, KWAttributeStats* attributeStats) const;

	// Creation de la grille cible a partir du produit cartesien des partitions
	// univariees des attributs de la grille source
	// On utilise au plus log2(N) attributs
	// On renvoie true si on a pu construire une grille avec au moins deux atrributs
	// Ajout de la transmission de la granularite a la grille cible
	boolean BuildDataGridFromClassStats(KWDataGrid* targetDataGrid, KWClassStats* classStats) const;

	// Creation d'un attribut de grille a partir d'une partition univariee deja stockee
	// Le parametrage de l'attribut source (granularite, poubelle si categoriel) est copie vers celui de l'attribut
	// de DataGrid
	void BuildDataGridAttributeFromUnivariateStats(KWDGAttribute* targetAttribute,
						       KWAttributeStats* attributeStats) const;

	///////////////////////////////////////////////////////////////////////////
	// Service de calcul des partitions univariees a la granularite courante
	// pour initialiser la grille cible (de facon compatible avec la grille source)
	// Dans toutes ces methodes, l egalite entre la granularite de la sourceDataGrid et celle de la targetDataGrid
	// est assuree

	// Creation d'un attribut de grille a partir d'une partition univariee calculee pour la granularite de
	// l'attribut source
	void BuildDataGridAttributeFromGranularizedPartition(KWDGAttribute* sourceAttribute,
							     KWDGAttribute* targetAttribute,
							     KWClassStats* classStats) const;

	// Creation des parties de l'attribut cible numerique selon une partition univariee specifiee dans une table
	// d'effectifs
	void BuildPartsOfContinuousAttributeFromFrequencyTable(KWDGAttribute* targetAttribute,
							       KWFrequencyTable* kwftTable,
							       const ALString& sAttributeName) const;

	// Creation des parties de l'attribut cible categoriel selon un vecteur de correspondance decrivant un groupage
	// univarie
	void BuildPartsOfSymbolAttributeFromGroupsIndex(KWDGAttribute* targetAttribute, const IntVector* ivGroups,
							int nGroupNumber, int nGarbageModalityNumber,
							const ALString& sAttributeName) const;

	// Creation de la grille cible a partir du produit cartesien des partitions
	// univariees des attributs de la grille source
	// Ces partitions sont les partitions optimales pour la granularite courante.
	// (calculees pour l'occasion)
	// On utilise au plus log2(N) attributs
	// On renvoie true si on a pu construire une grille avec au moins deux atrributs
	boolean BuildDataGridFromUnivariateProduct(KWDataGrid* targetDataGrid, KWClassStats* classStats) const;

	///////////////////////////////////////////////////////////////////////////
	// Service de creation d'une table d'effectifs a partir d'un attribut
	// Export d'un attribut sous la forme d'une table d'effectifs
	// CH V9 TODO MB
	// Pas le meme algo de construction que methodes
	// {KWDGPOGrouper,KWDGPDiscretizer}::InitializeFrequencyTableFromDataGrid A changer ? A mutualiser ?
	void ExportFrequencyTableFromOneAttribute(const KWFrequencyVector* kwfvCreator,
						  KWFrequencyTable* kwFrequencyTable,
						  const ALString& sAttributeName) const;

	///////////////////////////////////////////////////////////////////////////
	// Test de compatibilite de la grille cible avec la grille source
	// Le contenu de la cible doit etre une sous-partie du contenu de la source:
	//   - les attributs cibles appartiennent tous a la source
	//   - les parties cibles forment une partition des parties sources
	//   - les cellules cibles sont consistantes avec les cellules sources

	// Test de compatibilite complet
	boolean CheckDataGrid(const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite de la granularite
	boolean CheckGranularity(const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des valeurs cibles
	boolean CheckTargetValues(const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des attributs, qui doivent etre une sous partie
	// des attributs sources
	boolean CheckAttributes(const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des parties d'attributs, qui doivent former une partition
	// des partie sources
	boolean CheckParts(const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des cellules
	boolean CheckCells(const KWDataGrid* targetDataGrid) const;

	//////////////////////////////////////////////////////////////////////
	// Services generiques

	// Verification du parametrage
	boolean Check() const override;

	// Methode de test
	// Inclut un test d'export avec granularisation
	static void Test(const KWDataGrid* dataGrid);

	/////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Export des effectif des valeurs de la grille initiale vers un attribut categoriel entierement specifie
	// La valeur speciale recoit pour effectif l'ensemble des effectifs manquants
	void ExportSymbolAttributeValueFrequencies(KWDGAttribute* targetAttribute) const;

	// Tri des parties d'un attribut source symbolique, selon les groupements
	// de ces parties dans un attribut cible compatible
	// Les parties sources se trouvent dans le tableau resultat, trie correctement
	// par groupe, et en ordre aleatoire a l'interieur de chaque groupe
	//    oaSortedSourceParts: parties sources triees par groupe
	//    oaSortedGroupedParts: parties groupees associees aux parties source
	void SortAttributeParts(KWDGAttribute* sourceAttribute, KWDGAttribute* groupedAttribute,
				ObjectArray* oaSortedSourceParts, ObjectArray* oaSortedGroupedParts) const;

	// Initialisation d'un vecteur de nombres aleatoires compris entre 0 et max exclu, ordonnes et tous differents
	void InitRandomIndexVector(IntVector* ivRandomIndexes, int nIndexNumber, int nMaxIndex) const;

	// Grille de donnees source
	const KWDataGrid* sourceDataGrid;
};
