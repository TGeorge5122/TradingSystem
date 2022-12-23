#include "datagenerator.hpp"
#include "products.hpp"
#include "bonduniverseservice.hpp"
#include "bondmarketdataservice.hpp"
#include "bondpricingservice.hpp"
#include "bondtradebookingservice.hpp"
#include "bondinquiryservice.hpp"
#include "bondpositionservice.hpp"
#include "bondriskservice.hpp"
#include "bondalgoexecutionservice.hpp"
#include "bondexecutionservice.hpp"
#include "bondalgostreamingservice.hpp"
#include "bondstreamingservice.hpp"
#include "guiservice.hpp"
#include "bondhistoricaldataservice.hpp"

int main() {

	std::vector<TreasuryPrices> halfspreads;

	halfspreads.push_back(TreasuryPrices(0, 0, 1));
	halfspreads.push_back(TreasuryPrices(0, 0, 2));
	halfspreads.push_back(TreasuryPrices(0, 0, 3));
	halfspreads.push_back(TreasuryPrices(0, 0, 4));

	Bond bond_two("91282CFX4", CUSIP, "T", 4.5, "20241130");
	Bond bond_three("91282CGA3", CUSIP, "T", 4.0, "20251215");
	Bond bond_five("91282CFZ9", CUSIP, "T", 3.875, "20271130");
	Bond bond_seven("91282CFY2", CUSIP, "T", 3.875, "20291130");
	Bond bond_ten("91282CFV8", CUSIP, "T", 4.125, "20321130");
	Bond bond_twenty("912810TM0", CUSIP, "T", 4.0, "20421115");
	Bond bond_thirty("912810TL2", CUSIP, "T", 4.0, "20521115");

	BondUniverseService bond_uni_service;

	bond_uni_service.OnMessage(bond_two);
	bond_uni_service.OnMessage(bond_three);
	bond_uni_service.OnMessage(bond_five);
	bond_uni_service.OnMessage(bond_seven);
	bond_uni_service.OnMessage(bond_ten);
	bond_uni_service.OnMessage(bond_twenty);
	bond_uni_service.OnMessage(bond_thirty);

	BondGenerator g(bond_uni_service.GetUniverse(), halfspreads);

	g.generateMarketData(1e6, 5);

	g.generatePrices(1e6);

	g.generateTrades(10);

	g.generateInquiries(10);

	BondMarketDataService md_service;
	BondMarketDataConnector md_connector(&md_service, &bond_uni_service);

	BondPricingService prc_service;
	BondPricingConnector prc_connector(&prc_service, &bond_uni_service);

	BondTradeBookingService btb_service;
	BondTradeBookingConnector btb_connector(&btb_service, &bond_uni_service);

	BondInquiryConnector inq_connector;
	BondInquiryService inq_service(&inq_connector);
	inq_connector.setBondInquiryService(&inq_service);
	inq_connector.setBondUniverseService(&bond_uni_service);
	BondInquiryServiceListener inq_listener(&inq_service);
	inq_service.AddListener(&inq_listener);

	BondPositionService pos_service;
	BondRiskService risk_service;

	BondTradeBookingServiceListener trade_book_listener(&pos_service);
	btb_service.AddListener(&trade_book_listener);
	BondPositionServiceListener pos_listener(&risk_service);
	pos_service.AddListener(&pos_listener);

	BondAlgoExecutionService algo_exec_service;
	BondMarketDataServiceListener md_listener(&algo_exec_service);
	md_service.AddListener(&md_listener);
	
	BondExecutionConnector exec_connector;
	BondExecutionService exec_service(&exec_connector);
	exec_connector.setBondExecutionService(&exec_service);
	BondAlgoExecutionServiceListener algo_exec_listener(&exec_service);
	algo_exec_service.AddListener(&algo_exec_listener);

	BondAlgoStreamingService algo_stream_service;
	BondPricingServiceListener prc_listener(&algo_stream_service);
	prc_service.AddListener(&prc_listener);

	BondStreamingConnector stream_connector;
	BondStreamingService stream_service(&stream_connector);
	stream_connector.setBondStreamingService(&stream_service);
	BondAlgoStreamingServiceListener algo_stream_listener(&stream_service);
	algo_stream_service.AddListener(&algo_stream_listener);
	
	GUIConnector gui_connector;
	GUIService gui_service(&gui_connector);
	gui_connector.setGUIService(&gui_service);
	BondPricingServiceToGUIListener prc_listener_gui(&gui_service);
	prc_service.AddListener(&prc_listener_gui);

	BondHistoricalDataConnector< ExecutionOrder<Bond> > historical_exe_connector;
	BondHistoricalDataConnector< Inquiry<Bond> > historical_inq_connector;
	BondHistoricalDataConnector< Position<Bond> > historical_pos_connector;
	BondHistoricalDataConnector< PriceStream<Bond> > historical_ps_connector;
	BondHistoricalDataConnector< PV01<Bond> > historical_pv_connector;
		
	BondHistoricalDataService< ExecutionOrder<Bond> > historical_exe_service(&historical_exe_connector);
	BondHistoricalDataService< Inquiry<Bond> > historical_inq_service (&historical_inq_connector);
	BondHistoricalDataService< Position<Bond> > historical_pos_service (&historical_pos_connector);
	BondHistoricalDataService< PriceStream<Bond> > historical_ps_service (&historical_ps_connector);
	BondHistoricalDataService< PV01<Bond> > historical_pv_service (&historical_pv_connector);
	
	historical_exe_connector.setBondService(&historical_exe_service);
	historical_inq_connector.setBondService(&historical_inq_service);
	historical_pos_connector.setBondService(&historical_pos_service);
	historical_ps_connector.setBondService(&historical_ps_service);
	historical_pv_connector.setBondService(&historical_pv_service);	

	ServiceListenerToHistorical< ExecutionOrder<Bond> > exe_listener_historical(&historical_exe_service);
	ServiceListenerToHistorical< Inquiry<Bond> > inq_listener_historical(&historical_inq_service);
	ServiceListenerToHistorical< Position<Bond> > pos_listener_historical(&historical_pos_service);
	ServiceListenerToHistorical< PriceStream<Bond> > ps_listener_historical(&historical_ps_service);
	ServiceListenerToHistorical< PV01<Bond> > pv_listener_historical(&historical_pv_service);

	exec_service.AddListener(&exe_listener_historical);
	stream_service.AddListener(&ps_listener_historical);
	inq_service.AddListener(&inq_listener_historical);
	pos_service.AddListener(&pos_listener_historical);
	risk_service.AddListener(&pv_listener_historical);
	
	md_connector.Subscribe();
	prc_connector.Subscribe();
	btb_connector.Subscribe();
	inq_connector.Subscribe();
}