#pragma once

#include "Containers/String.h"

#include "NsGui/Panel.h"
#include "NsGui/UICollection.h"
#include "NsGui/ColumnDefinition.h"
#include "NsGui/RowDefinition.h"
#include "NsGui/Grid.h"
#include "NsGui/FrameworkElement.h"
#include "NsGui/Label.h"

FORCEINLINE void AddColumnDefinition(Noesis::ColumnDefinitionCollection* pColumnCollection, float width, Noesis::GridUnitType unit)
{
	using namespace Noesis;

	GridLength gl = GridLength(width, unit);
	Ptr<ColumnDefinition> col = *new ColumnDefinition();
	col->SetWidth(gl);
	pColumnCollection->Add(col);
}

FORCEINLINE void AddRowDefinition(Noesis::RowDefinitionCollection* pRowCollection, float height, Noesis::GridUnitType unit)
{
	using namespace Noesis;

	GridLength gl = GridLength(height, unit);
	Ptr<RowDefinition> row = *new RowDefinition();
	row->SetHeight(gl);
	pRowCollection->Add(row);
}