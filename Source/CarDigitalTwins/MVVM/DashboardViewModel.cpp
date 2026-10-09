#include "MVVM/DashboardViewModel.h"

const TCHAR* GetTwinDashboardViewName(ETwinDashboardView View)
{
	switch (View)
	{
	case ETwinDashboardView::Powertrain: return TEXT("Powertrain");
	case ETwinDashboardView::Tyres: return TEXT("Tyres");
	case ETwinDashboardView::Body: return TEXT("Body");
	default: return TEXT("Overview");
	}
}

void UDashboardViewModel::SelectView(ETwinDashboardView NewActiveView)
{
	if (NewActiveView < ETwinDashboardView::Count)
	{
		SetField(EDashboardViewModelField::ActiveView, ActiveView, NewActiveView);
	}
}