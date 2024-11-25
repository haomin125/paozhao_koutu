
#include <string>
#include <algorithm>

#include "xj_app_web_server.h"
#include "camera_manager.h"
#include "logger.h"
#include "database.h"
#include "db_utils.h"
#include "running_status.h"
#include "shared_utils.h"
#include "xj_app_workflow.h"
#include "customized_json_config.h"
#include "xj_app_io_manager.h"

#include <boost/optional/optional.hpp>
#include <boost/foreach.hpp>

using namespace std;
using namespace boost::property_tree;

AppWebServer::AppWebServer(const shared_ptr<CameraManager> pCameraManager) : ApbWebServer(pCameraManager)
{
		
	addWebCmds();
}


AppWebServer::AppWebServer(const shared_ptr<CameraManager> pCameraManager, const string &addr, const unsigned short port) : 
	ApbWebServer(pCameraManager, addr, port)
{
	addWebCmds();

	
	mapStringValues["all"] = StringValue::all;
	mapStringValues["cropImage"] = StringValue::cropImage;
	mapStringValues["rotationOfTD"] = StringValue::rotationOfTD;
}

int AppWebServer::addWebCmds()
{
	LogINFO << "Add APP web server commands...";

	CreateTestProduction();

	GetDetectingParams();//获取瑕疵检测参数
	
	SetRect();

	SetParams();

	SetImagePath();//设置浏览图片路径

	GetImagePath();//获取浏览图片路径

	SettingWrite();//保存参数到数据库

	GetPlcParams();

	SetPlcParams();

	SetDefaultAutoParams();

	Preview();

	return 0;
}


int AppWebServer::CreateTestProduction()
{
	/*
   POST: http://localhost:8080/create_test_prod?prod=CYX&board=0&camera=0
   Return: Success
   three settings could be empty, which means it is a new test production without any existing setting
   process phase: 0 no porcess at all, 1 pre-process only, 2 pre-process and classification
 */
	m_server.resource["^/create_test_prod"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		auto query_fields = request->parse_query_string();
		try
		{
			string prod = SimpleWeb::getValue(query_fields, "prod");
			ptree pt;
			read_json(request->content, pt);

			// set current test product info and reset all test related flags to false
			RunningInfo::instance().GetTestProductionInfo().SetCurrentProd(prod, false);
			RunningInfo::instance().GetTestProductionInfo().SetTestStatus(0);
			RunningInfo::instance().GetTestProductionInfo().SetTriggerCameraImageStatus(0);

			// load product setting, and other test related values
			ProductSetting::PRODUCT_SETTING setting = RunningInfo::instance().GetTestProductSetting().GetSettings();
			if (!pt.empty())
			{
				setting.loadFromJson(pt);
			}
			else
			{
				setting.reset();
			}
			RunningInfo::instance().GetTestProductSetting().setPhase(30);
			RunningInfo::instance().GetTestProductSetting().UpdateCurrentProductSetting(setting);
			LogINFO << "Create test product " << prod << " and setting successfully";

			response->write("Success");
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}
		return 0;
	};
	return 0;
}


int AppWebServer::GetDetectingParams()
{
	/*	
	GET:http://localhost:8080/get_detecting_parameters?prod=CYX&board=0&camera=0
	Return detecting_parameters:{	
		rectTips:
		[
			{ txt: '提示文本', img: '提示图片的地址' },
			{ txt: '提示文本', img: '提示图片的地址' },
			{ txt: '提示文本', img: '提示图片的地址' },
			{ txt: '提示文本', img: '提示图片的地址' },
			{ txt: '提示文本', img: '提示图片的地址' }
		]，
		rect:	
		[
			{
				"x": "1.0",
				"y": "2.0",
				"width": "3.0",
				"height": "4.0"
			},
			{
				"x": "1.0",
				"y": "2.0",
				"width": "3.0",
				"height": "4.0"
			},
			{
				"x": "1.0",
				"y": "2.0",
				"width": "3.0",
				"height": "4.0"
			},
			{
				"x": "1.0",
				"y": "2.0",
				"width": "3.0",
				"height": "4.0"
			},
			{
				"x": "1.0",
				"y": "2.0",
				"width": "3.0",
				"height": "4.0"
			}
		],
		"params_boxSetting": {
			"params_okBox": {
				"enable": "1"
			}
    	},
		"params_firstDet": {
			"params_dimension": {
				"enable": "1",
				"parameters": {
					"baseDimension": "67.5",
					"upTolerance": "0.4",
					"lowTolerance": "0.4",
					"leftWidthMeasure": "72.5",
					"rightWidthMeasure": "72.5",
					"leftWidthRatio": "0.037",
					"rightWidthRatio": "0.037"
				}
			},
			"params_blackSpotDefect": {
				"enable": "1",
				"parameters": {
					"blackSpotDefectSensitivity": "20",
					"blackSpotDefectMinArea": "30"
				}
			},
			"params_whiteEdgeDefect": {
				"enable": "1",
				"parameters": {
					"whiteEdgeDefectSensitivity": "70",
					"whiteEdgeDefectMinArea": "35"
				}
			}
		},
		"params_secondDet": {
			"params_spotDefect": {
				"enable": "1",
				"parameters": {
					"spotDefectSensitivity": "35",
					"spotDefectMinArea": "55",
					"spotDefectMinLength": "10"
				}
			},
			"params_scratchDefect": {
				"enable": "1",
				"parameters": {
					"scratchDefectSensitivity": "5",
					"scratchDefectMinRatio": "10",
					"scratchDefectMinLength": "40"
				}
			},
			"params_weekDefect": {
				"enable": "1",
				"parameters": {
					"weekDefectCenterSize": "2",
					"weekDefectCenterSensitivity": "50",
					"weekDefectCenterMinNum": "3"
				}
			}
		},
		"imagePath": "N\/A"
	}
	*/

	m_server.resource["^/get_detecting_parameters"]["GET"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		auto query_fields = request->parse_query_string();
		try
		{
			ProductSetting::PRODUCT_SETTING setting = RunningInfo::instance().GetTestProductSetting().GetSettings();	
			string prod = SimpleWeb::getValue(query_fields, "prod");
			if (!prod.empty())
			{
				Database db;
				if (db.prod_setting_read(prod, setting))
				{
					LogERROR << "Read product setting in database failed";
					response->write(SimpleWeb::StatusCode::server_error_internal_server_error, "Read Product Setting Failed");
				}
			}

			string board = SimpleWeb::getValue(query_fields, "board");
			string cam = SimpleWeb::getValue(query_fields, "camera");
			const int boardId = stoi(board);
			const int camId = stoi(cam);
			LogINFO << "get_detecting_parameters: prod=" << prod << " board=" << board << " camera=" << cam;

			
			boost::property_tree::ptree root;

			//0.提示图片
			boost::property_tree::ptree rect_tips;
			//0-1检测框
			boost::property_tree::ptree det_box_roi_tip;
			det_box_roi_tip.put_child("txt", boost::property_tree::ptree("框选药板检测区域"));
			det_box_roi_tip.put_child("img", boost::property_tree::ptree("/opt/app/pics/detBox.png"));
			rect_tips.push_back(make_pair("", det_box_roi_tip));

			//0-2尺寸框
			boost::property_tree::ptree dim_box_roi_tip;
			dim_box_roi_tip.put_child("txt", boost::property_tree::ptree("框选批号检测区域"));
			dim_box_roi_tip.put_child("img", boost::property_tree::ptree("/opt/app/pics/dimBox.png"));
			rect_tips.push_back(make_pair("", dim_box_roi_tip));
			root.put_child("rectTips", rect_tips);

			//1.ROI区域框
			boost::property_tree::ptree rectArr;
			//1）检测框
			boost::property_tree::ptree rectArrDet;
			int rectIdx = int(ProductSettingFloatMapper::BOX_START_INDEX) + camId * (4+8) * 4;	
			LogINFO << "board[" <<  boardId << "] CAM" << camId << " det box index: " << rectIdx;
			int rectCount = setting.float_settings[int(ProductSettingFloatMapper::BOARD_COUNT_PER_VIEW)];				
			for (int i = 0; i < rectCount; i++)
			{
				boost::property_tree::ptree rect;
				boost::property_tree::ptree rect_x(to_string(setting.float_settings[rectIdx + 0 + i * 4]));
				boost::property_tree::ptree rect_y(to_string(setting.float_settings[rectIdx + 1 + i * 4]));
				boost::property_tree::ptree rect_width(to_string(setting.float_settings[rectIdx + 2 + i * 4]));
				boost::property_tree::ptree rect_height(to_string(setting.float_settings[rectIdx + 3 + i * 4]));				
				rect.put_child("x", rect_x);
				rect.put_child("y", rect_y);
				rect.put_child("width", rect_width);
				rect.put_child("height", rect_height);
				rectArrDet.push_back(make_pair("", rect));
			}
			rectArr.push_back(make_pair("", rectArrDet));

			//1）批号Roi
			int lotNumCount = setting.float_settings[int(ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD)];	
			boost::property_tree::ptree rectArrPlot;
			rectIdx += 4 * 4;				
			for (int i = 0; i < rectCount*lotNumCount; i++)
			{
				boost::property_tree::ptree rect;
				boost::property_tree::ptree rect_x(to_string(setting.float_settings[rectIdx + 0 + i * 4]));
				boost::property_tree::ptree rect_y(to_string(setting.float_settings[rectIdx + 1 + i * 4]));
				boost::property_tree::ptree rect_width(to_string(setting.float_settings[rectIdx + 2 + i * 4]));
				boost::property_tree::ptree rect_height(to_string(setting.float_settings[rectIdx + 3 + i * 4]));				
				rect.put_child("x", rect_x);
				rect.put_child("y", rect_y);
				rect.put_child("width", rect_width);
				rect.put_child("height", rect_height);
				rectArrPlot.push_back(make_pair("", rect));
			}
			rectArr.push_back(make_pair("", rectArrPlot));

			root.put_child("rect", rectArr);


			boost::property_tree::ptree params_detect;

			boost::property_tree::ptree params_cropImage;
			boost::property_tree::ptree dimensionDet_enable(to_string(1));
			params_cropImage.put_child("enable", dimensionDet_enable);
			
			
			boost::property_tree::ptree BOARD_COUNT_PER_VIEW(to_string(setting.float_settings[int(ProductSettingFloatMapper::BOARD_COUNT_PER_VIEW)]));
			boost::property_tree::ptree TABLET_COUNT_PER_BOARD(to_string(setting.float_settings[int(ProductSettingFloatMapper::TABLET_COUNT_PER_BOARD)]));
			boost::property_tree::ptree TABLET_ROI_COL_COUNT(to_string(setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_COL_COUNT)]));
			boost::property_tree::ptree TABLET_ROI_ROW_COUNT(to_string(setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_ROW_COUNT)]));
			boost::property_tree::ptree TABLET_ROI_COL_OFFSET(to_string(setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_COL_OFFSET)]));
			boost::property_tree::ptree TABLET_ROI_ROW_OFFSET(to_string(setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_ROW_OFFSET)]));
			boost::property_tree::ptree LOTNUM_COUNT_PER_BOARD(to_string(setting.float_settings[int(ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD)]));
			boost::property_tree::ptree BANMIANBUQUAN_COLS(to_string(setting.float_settings[int(ProductSettingFloatMapper::BANMIANBUQUAN_COLS)]));
			boost::property_tree::ptree LOT_POSITION(to_string(setting.float_settings[int(ProductSettingFloatMapper::LOT_POSITION)]));

			
			boost::property_tree::ptree HSV_H_LOWER(to_string(setting.float_settings[int(ProductSettingFloatMapper::HSV) + 0 + boardId*6]));
			boost::property_tree::ptree HSV_H_UPPER(to_string(setting.float_settings[int(ProductSettingFloatMapper::HSV) + 1 + boardId*6]));
			boost::property_tree::ptree HSV_S_LOWER(to_string(setting.float_settings[int(ProductSettingFloatMapper::HSV) + 2 + boardId*6]));
			boost::property_tree::ptree HSV_S_UPPER(to_string(setting.float_settings[int(ProductSettingFloatMapper::HSV) + 3 + boardId*6]));
			boost::property_tree::ptree HSV_V_LOWER(to_string(setting.float_settings[int(ProductSettingFloatMapper::HSV) + 4 + boardId*6]));
			boost::property_tree::ptree HSV_V_UPPER(to_string(setting.float_settings[int(ProductSettingFloatMapper::HSV) + 5 + boardId*6]));

			boost::property_tree::ptree HSV_H;
			boost::property_tree::ptree HSV_S;			
			boost::property_tree::ptree HSV_V;

			HSV_H.push_back(make_pair("", HSV_H_LOWER));
			HSV_H.push_back(make_pair("", HSV_H_UPPER));
			HSV_S.push_back(make_pair("", HSV_S_LOWER));
			HSV_S.push_back(make_pair("", HSV_S_UPPER));
			HSV_V.push_back(make_pair("", HSV_V_LOWER));
			HSV_V.push_back(make_pair("", HSV_V_UPPER));

			boost::property_tree::ptree cropImage_parameters;
			cropImage_parameters.put_child("HSV_H", HSV_H);
			cropImage_parameters.put_child("HSV_S", HSV_S);
			cropImage_parameters.put_child("HSV_V", HSV_V);
			cropImage_parameters.put_child("BOARD_COUNT_PER_VIEW", BOARD_COUNT_PER_VIEW);
			cropImage_parameters.put_child("TABLET_COUNT_PER_BOARD", TABLET_COUNT_PER_BOARD);
			cropImage_parameters.put_child("TABLET_ROI_COL_COUNT", TABLET_ROI_COL_COUNT);
			cropImage_parameters.put_child("TABLET_ROI_ROW_COUNT", TABLET_ROI_ROW_COUNT);
			cropImage_parameters.put_child("TABLET_ROI_COL_OFFSET", TABLET_ROI_COL_OFFSET);
			cropImage_parameters.put_child("TABLET_ROI_ROW_OFFSET", TABLET_ROI_ROW_OFFSET);	
			cropImage_parameters.put_child("LOTNUM_COUNT_PER_BOARD", LOTNUM_COUNT_PER_BOARD);	
			cropImage_parameters.put_child("BANMIANBUQUAN_COLS", BANMIANBUQUAN_COLS);		
			cropImage_parameters.put_child("LOT_POSITION", LOT_POSITION);	
			params_cropImage.put_child("parameters", cropImage_parameters);		
			params_detect.put_child("params_cropImage", params_cropImage);

			boost::property_tree::ptree params_model;
			boost::property_tree::ptree model_enable(to_string(1));
			params_model.put_child("enable", model_enable);
			boost::property_tree::ptree SENSITIVITY(to_string(setting.float_settings[int(ProductSettingFloatMapper::SENSITIVITY)]));
			boost::property_tree::ptree MODEL_WIDTH(to_string(setting.float_settings[int(ProductSettingFloatMapper::MODEL_WIDTH)]));
			boost::property_tree::ptree MODEL_HEIGHT(to_string(setting.float_settings[int(ProductSettingFloatMapper::MODEL_HEIGHT)]));
			boost::property_tree::ptree MODEL_CATEGORY(to_string(setting.float_settings[int(ProductSettingFloatMapper::MODEL_CATEGORY)]));
			boost::property_tree::ptree model_parameters;
			model_parameters.put_child("SENSITIVITY", SENSITIVITY);
			model_parameters.put_child("MODEL_WIDTH", MODEL_WIDTH);
			model_parameters.put_child("MODEL_HEIGHT", MODEL_HEIGHT);
			model_parameters.put_child("MODEL_CATEGORY", MODEL_CATEGORY);		
			params_model.put_child("parameters", model_parameters);		
			params_detect.put_child("params_model", params_model);

			boost::property_tree::ptree params_signal;
			boost::property_tree::ptree signal_enable(to_string(1));
			params_signal.put_child("enable", signal_enable);
			boost::property_tree::ptree TEST_TYPE(to_string(setting.float_settings[int(ProductSettingFloatMapper::TEST_TYPE)]));
			boost::property_tree::ptree TEST_CHANNEL(to_string(setting.float_settings[int(ProductSettingFloatMapper::TEST_CHANNEL)]));
			boost::property_tree::ptree signal_parameters;
			signal_parameters.put_child("TEST_TYPE", TEST_TYPE);
			signal_parameters.put_child("TEST_CHANNEL", TEST_CHANNEL);		
			params_signal.put_child("parameters", signal_parameters);		
			params_detect.put_child("params_signal", params_signal);

			
			boost::property_tree::ptree params_defect;
			boost::property_tree::ptree defect_enable(to_string(1));
			params_defect.put_child("enable", defect_enable);
			
						
			boost::property_tree::ptree defect_parameters;
			// cout<<defect_class.size()<<" =======================111"<<endl;
			for (size_t i = 0; i < 12; i++)
			{
				boost::property_tree::ptree temp(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CUCAO) + i]));
				
				defect_parameters.put_child("DEFECT_"+to_string(i), temp);
				// cout<< int(ProductSettingFloatMapper::DEFECT_CUCAO)+i <<" ======  "<<to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CUCAO)+i])<<": "<<"DEFECT_"+to_string(i)<<endl;
				
			}
			params_defect.put_child("parameters", defect_parameters);		
			params_detect.put_child("params_defect", params_defect);
			
			
			
			// boost::property_tree::ptree DEFECT_CUCAO(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CUCAO)]));
			// boost::property_tree::ptree DEFECT_HEIDIAN(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_HEIDIAN)]));
			// boost::property_tree::ptree DEFECT_JIAFEN(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_JIAFEN)]));
			// boost::property_tree::ptree DEFECT_JIETOU(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_JIETOU)]));
			// boost::property_tree::ptree DEFECT_KELI(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_KELI)]));
			// boost::property_tree::ptree DEFECT_LOULVBO(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LOULVBO)]));
			// boost::property_tree::ptree DEFECT_LVBOPOSUN(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LVBOPOSUN)]));
			// boost::property_tree::ptree DEFECT_POSUN(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_POSUN)]));
			// boost::property_tree::ptree DEFECT_QUEJIAO(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_QUEJIAO)]));
			// boost::property_tree::ptree DEFECT_REFENGBULIANG(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_REFENGBULIANG)]));
			// boost::property_tree::ptree DEFECT_YIWU(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_YIWU)]));
			
			// boost::property_tree::ptree defect_parameters;
			// defect_parameters.put_child("DEFECT_CUCAO", DEFECT_CUCAO);
			// defect_parameters.put_child("DEFECT_HEIDIAN", DEFECT_HEIDIAN);
			// defect_parameters.put_child("DEFECT_JIAFEN", DEFECT_JIAFEN);
			// defect_parameters.put_child("DEFECT_JIETOU", DEFECT_JIETOU);
			// defect_parameters.put_child("DEFECT_KELI", DEFECT_KELI);
			// defect_parameters.put_child("DEFECT_LOULVBO", DEFECT_LOULVBO);
			// defect_parameters.put_child("DEFECT_LVBOPOSUN", DEFECT_LVBOPOSUN);
			// defect_parameters.put_child("DEFECT_POSUN", DEFECT_POSUN);
			// defect_parameters.put_child("DEFECT_QUEJIAO", DEFECT_QUEJIAO);
			// defect_parameters.put_child("DEFECT_REFENGBULIANG", DEFECT_REFENGBULIANG);
			// defect_parameters.put_child("DEFECT_YIWU", DEFECT_YIWU);


			// params_defect.put_child("parameters", defect_parameters);		
			// params_detect.put_child("params_defect", params_defect);

			boost::property_tree::ptree params_jiaonang;
			boost::property_tree::ptree jiaonang_enable(to_string(setting.float_settings[int(ProductSettingFloatMapper::USE_JIAONANG)]));
			params_jiaonang.put_child("enable", jiaonang_enable);
			params_detect.put_child("params_jiaonang", params_jiaonang);


			boost::property_tree::ptree params_usepihao;
			boost::property_tree::ptree usepihao_enable(to_string(setting.float_settings[int(ProductSettingFloatMapper::USE_PIHAO)]));
			params_usepihao.put_child("enable", usepihao_enable);
			params_detect.put_child("params_usepihao", params_usepihao);

			boost::property_tree::ptree params_pngconvert;
			boost::property_tree::ptree pngconvert_enable(to_string(setting.float_settings[int(ProductSettingFloatMapper::PNG_CONVERT)]));
			params_pngconvert.put_child("enable", pngconvert_enable);
			params_detect.put_child("params_pngconvert", params_pngconvert);

			boost::property_tree::ptree params_usemodel;
			boost::property_tree::ptree usemodel_enable(to_string(setting.float_settings[int(ProductSettingFloatMapper::USE_MODEL)]));
			params_usemodel.put_child("enable", usemodel_enable);
			params_detect.put_child("params_usemodel", params_usemodel);

			root.put_child("params_detect", params_detect);

// //灵敏度设置

// 			boost::property_tree::ptree params_sensitivity;
// 			boost::property_tree::ptree sensitivity_enable(to_string(1));
// 			params_sensitivity.put_child("enable", sensitivity_enable);
// 			boost::property_tree::ptree DEFECT_GOOD(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_GOOD)]));
// 			boost::property_tree::ptree DEFECT_ZHEZHOU(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_ZHEZHOU)]));
// 			boost::property_tree::ptree DEFECT_PIHAO(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_PIHAO)]));
// 			boost::property_tree::ptree DEFECT_KONGLI(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_KONGLI)]));
// 			boost::property_tree::ptree DEFECT_BANLI(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_BANLI)]));
// 			boost::property_tree::ptree DEFECT_JUPI(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_JUPI)]));
// 			boost::property_tree::ptree DEFECT_CUOWEI(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CUOWEI)]));
// 			boost::property_tree::ptree DEFECT_CXBL(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CXBL)]));
// 			boost::property_tree::ptree DEFECT_MAOFA(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_MAOFA)]));
// 			boost::property_tree::ptree DEFECT_LIEWEN(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LIEWEN)]));
// 			boost::property_tree::ptree DEFECT_SHUANGP(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_SHUANGP)]));
// 			boost::property_tree::ptree DEFECT_LOUFEN(to_string(setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LOUFEN)]));
			
// 			boost::property_tree::ptree sensitivity_parameters;
// 			sensitivity_parameters.put_child("DEFECT_GOOD", DEFECT_GOOD);
// 			sensitivity_parameters.put_child("DEFECT_ZHEZHOU", DEFECT_ZHEZHOU);
// 			sensitivity_parameters.put_child("DEFECT_PIHAO", DEFECT_PIHAO);
// 			sensitivity_parameters.put_child("DEFECT_KONGLI", DEFECT_KONGLI);
// 			sensitivity_parameters.put_child("DEFECT_BANLI", DEFECT_BANLI);
// 			sensitivity_parameters.put_child("DEFECT_JUPI", DEFECT_JUPI);
// 			sensitivity_parameters.put_child("DEFECT_CUOWEI", DEFECT_CUOWEI);
// 			sensitivity_parameters.put_child("DEFECT_CXBL", DEFECT_CXBL);
// 			sensitivity_parameters.put_child("DEFECT_MAOFA", DEFECT_MAOFA);
// 			sensitivity_parameters.put_child("DEFECT_LIEWEN", DEFECT_LIEWEN);
// 			sensitivity_parameters.put_child("DEFECT_SHUANGP", DEFECT_SHUANGP);
// 			sensitivity_parameters.put_child("DEFECT_LOUFEN", DEFECT_LOUFEN);

	

// 			params_sensitivity.put_child("parameters", sensitivity_parameters);		
// 			params_detect.put_child("params_sensitivity", params_sensitivity);
// /////////////////////////////////////////



			root.put_child("params_detect", params_detect);

			//4.图片路径
			boost::property_tree::ptree imagePath(RunningInfo::instance().GetTestProductSetting().getImageName());
			root.put_child("imagePath", imagePath);

			//以字符流形式发送给前端
			stringstream ss;
			write_json(ss, root);
			response->write(ss);

		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
		}
	};
	return 0;
}


int AppWebServer::SetRect()
{
	/*
	POST: http://localhost:8080/set_rect?function=setRect&board=0&camera=0
	example:
	{
		"locating_roi": {
			"x": "1.0",
			"y": "2.0",
			"width": "3.0",
			"height": "4.0"
		},
		"locating_model_rect": {
			"x": "1.0",
			"y": "2.0",
			"width": "3.0",
			"height": "4.0"
		},
		"three_date_roi": {
			"x": "1.0",
			"y": "2.0",
			"width": "3.0",
			"height": "4.0"
		}
	}
   Return: Success
 */
	m_server.resource["^/set_rect"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		string prod = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
		auto query_fields = request->parse_query_string();
		try
		{
			string function = SimpleWeb::getValue(query_fields, "function");
			string board = SimpleWeb::getValue(query_fields, "board");
			string cam = SimpleWeb::getValue(query_fields, "camera");
			LogINFO << "set_rect: prod=" << prod << " board=" << board << " camera=" << cam;

			const int boardId = stoi(board);
			const int camId = stoi(cam);

			ptree pt;
			read_json(request->content, pt);// load product setting, and other test related values	
			if (pt.empty())
			{
				LogERROR << "Invalid content in test product setting";
				response->write(SimpleWeb::StatusCode::client_error_bad_request, "Invalid Content");
				return 1;
			}

			if (!pt.get_child_optional("rect"))
			{
				LogERROR << "find rect failed";
				response->write(SimpleWeb::StatusCode::client_error_bad_request, "find rect failed");
				return 1;
			}

			ptree boxeGroup = pt.get_child("rect"); // get_child得到数组对象
			// if(boxeGroup.size() != 3)
			// {
			// 	LogERROR << "fail to find 3 boxes group";
			// 	response->write(SimpleWeb::StatusCode::client_error_bad_request, "fail to find 3 boxes group");
			// 	return 1;
			// }

			ProductSetting::PRODUCT_SETTING setting = RunningInfo::instance().GetTestProductSetting().GetSettings();

			int rectIdx = int(ProductSettingFloatMapper::BOX_START_INDEX) + camId * (4+8) * 4;			
			BOOST_FOREACH (boost::property_tree::ptree::value_type &boxes, boxeGroup)// 遍历外层数组(检测框、尺寸框、屏蔽框)
			{	
				// cout << boxes.second.size() << endl;
				// setting.float_settings[int(ProductSettingFloatMapper::BOARD_COUNT_PER_VIEW)] = boxes.second.size();				
				BOOST_FOREACH (boost::property_tree::ptree::value_type &v, boxes.second)// 遍历一组框数量
				{
					ptree child = v.second;
					if (!child.get_child_optional("x") || !child.get_child_optional("y") || !child.get_child_optional("width") || !child.get_child_optional("height"))
					{
						LogERROR << "find x, y, width, height in det box rect failed";
						response->write(SimpleWeb::StatusCode::client_error_bad_request, "find x, y, width, height in det box rect failed");
						return 1;
					}

					setting.float_settings[rectIdx + 0] = child.get<float>("x");
					setting.float_settings[rectIdx + 1] = child.get<float>("y");
					setting.float_settings[rectIdx + 2] = child.get<float>("width");
					setting.float_settings[rectIdx + 3] = child.get<float>("height");
					rectIdx += 4;
				}
				rectIdx = int(ProductSettingFloatMapper::BOX_START_INDEX) + camId * (4+8) * 4 + 4*4;
			}

			RunningInfo::instance().GetTestProductSetting().UpdateCurrentProductSetting(setting);
			response->write("Success");
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}

		return 0;
	};

	return 0;
}


int AppWebServer::SetParams()
{
	/*
	POST: http://localhost:8080/set_params?group=firstDet&function=blackSpotDefect&board=0&camera=0
		{
		"enable": "true"
		}
	*/
	m_server.resource["^/set_params"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		
		auto query_fields = request->parse_query_string();
		try
		{
			boost::property_tree::ptree pt;
			read_json(request->content, pt);
			if (pt.empty())// load product setting, and other test related values
			{
				LogERROR << "Invalid content in test product setting";
				response->write(SimpleWeb::StatusCode::client_error_bad_request, "Invalid Content");
				return 1;
			}

			if (!pt.get_child_optional("enable"))
			{
				LogERROR << "find parameters setting failed";
				response->write(SimpleWeb::StatusCode::client_error_bad_request, "find parameters setting failed");
				return 1;
			}

			ProductSetting::PRODUCT_SETTING setting = RunningInfo::instance().GetTestProductSetting().GetSettings();
			string prod = RunningInfo::instance().GetProductionInfo().GetCurrentProd();	

			string board = SimpleWeb::getValue(query_fields, "board");
			string cam = SimpleWeb::getValue(query_fields, "camera");
			LogINFO << "set_params: prod=" << prod << " board=" << board << " camera=" << cam;

			const int boardId = stoi(board);
			const int camId = stoi(cam);

			string group = SimpleWeb::getValue(query_fields, "group");
			string function = SimpleWeb::getValue(query_fields, "function");

			if (group == "detect" && function == "cropImage")//第一次拍照尺寸
			{
				// setting.bool_settings[int(ProductSettingBooleanMapper::CAM_FIRST_DET_DIMENSION_ENABLE) + camId * int(ProductSettingBooleanMapper::MAX_PARMS_SETTING_BOOL_ID_PER_CAM)] = pt.get<bool>("enable");
				if (!pt.get_child_optional("parameters"))
				{
					LogERROR << "find cropImage failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				ptree parametersTree = pt.get_child("parameters");
				if (!parametersTree.get_child_optional("BOARD_COUNT_PER_VIEW") || !parametersTree.get_child_optional("TABLET_COUNT_PER_BOARD") || !parametersTree.get_child_optional("TABLET_ROI_COL_COUNT")|| 
					!parametersTree.get_child_optional("TABLET_ROI_ROW_COUNT") || !parametersTree.get_child_optional("TABLET_ROI_COL_OFFSET") ||
					!parametersTree.get_child_optional("TABLET_ROI_ROW_OFFSET") || !parametersTree.get_child_optional("LOTNUM_COUNT_PER_BOARD") || !parametersTree.get_child_optional("BANMIANBUQUAN_COLS")||
					!parametersTree.get_child_optional("LOT_POSITION") || !parametersTree.get_child_optional("HSV_H") || !parametersTree.get_child_optional("HSV_S") || !parametersTree.get_child_optional("HSV_V"))
				{
					LogERROR << "find BOARD_COUNT_PER_VIEW, TABLET_COUNT_PER_BOARD, TABLET_ROI_COL_COUNT, TABLET_ROI_ROW_COUNT, TABLET_ROI_COL_OFFSET, TABLET_ROI_ROW_OFFSET， LOTNUM_COUNT_PER_BOARD,BANMIANBUQUAN_COLS, LOT_POSITION HSV in parameters failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find BOARD_COUNT_PER_VIEW, TABLET_COUNT_PER_BOARD, TABLET_ROI_COL_COUNT, TABLET_ROI_ROW_COUNT, TABLET_ROI_COL_OFFSET, TABLET_ROI_ROW_OFFSET, LOTNUM_COUNT_PER_BOARD in parameters failed");
					return 1;
				}

				ptree HSV_H = parametersTree.get_child("HSV_H"); // get_child得到数组对象
				int hIndx(int(ProductSettingFloatMapper::HSV) + 0 + boardId*6);
				BOOST_FOREACH (boost::property_tree::ptree::value_type &h, HSV_H)
				{
					setting.float_settings[hIndx] = h.second.get<float>("");
					hIndx++;
				}
				ptree HSV_S = parametersTree.get_child("HSV_S"); // get_child得到数组对象
				int sIndx(int(ProductSettingFloatMapper::HSV) + 2 + boardId*6);
				BOOST_FOREACH (boost::property_tree::ptree::value_type &s, HSV_S)
				{
					setting.float_settings[sIndx] = s.second.get<float>("");
					sIndx++;
				}
				ptree HSV_V = parametersTree.get_child("HSV_V"); // get_child得到数组对象
				int vIndx(int(ProductSettingFloatMapper::HSV) + 4 + boardId*6);
				BOOST_FOREACH (boost::property_tree::ptree::value_type &v, HSV_V)
				{
					setting.float_settings[vIndx] = v.second.get<float>("");
					vIndx++;
				}


				setting.float_settings[int(ProductSettingFloatMapper::BOARD_COUNT_PER_VIEW)] = parametersTree.get<float>("BOARD_COUNT_PER_VIEW");
				setting.float_settings[int(ProductSettingFloatMapper::TABLET_COUNT_PER_BOARD)] = parametersTree.get<float>("TABLET_COUNT_PER_BOARD");
				setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_COL_COUNT)] = parametersTree.get<float>("TABLET_ROI_COL_COUNT");
				setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_ROW_COUNT)] = parametersTree.get<float>("TABLET_ROI_ROW_COUNT");
				setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_COL_OFFSET)] = parametersTree.get<float>("TABLET_ROI_COL_OFFSET");
				setting.float_settings[int(ProductSettingFloatMapper::TABLET_ROI_ROW_OFFSET)] = parametersTree.get<float>("TABLET_ROI_ROW_OFFSET");
				setting.float_settings[int(ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD)] = parametersTree.get<float>("LOTNUM_COUNT_PER_BOARD");
				setting.float_settings[int(ProductSettingFloatMapper::BANMIANBUQUAN_COLS)] = parametersTree.get<float>("BANMIANBUQUAN_COLS");
				setting.float_settings[int(ProductSettingFloatMapper::LOT_POSITION)] = parametersTree.get<float>("LOT_POSITION");
			}

			if (group == "detect" && function == "model")//第一次拍照尺寸
			{
				// setting.bool_settings[int(ProductSettingBooleanMapper::CAM_FIRST_DET_DIMENSION_ENABLE) + camId * int(ProductSettingBooleanMapper::MAX_PARMS_SETTING_BOOL_ID_PER_CAM)] = pt.get<bool>("enable");
				if (!pt.get_child_optional("parameters"))
				{
					LogERROR << "find model params failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				ptree parametersTree = pt.get_child("parameters");
				if (!parametersTree.get_child_optional("SENSITIVITY") || !parametersTree.get_child_optional("MODEL_WIDTH")|| 
					!parametersTree.get_child_optional("MODEL_HEIGHT") || !parametersTree.get_child_optional("MODEL_CATEGORY"))
				{
					LogERROR << "find SENSITIVITY, MODEL_WIDTH, MODEL_HEIGHT, MODEL_CATEGORY in parameters failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find SENSITIVITY, MODEL_WIDTH, MODEL_HEIGHT, MODEL_CATEGORY in parameters failed");
					return 1;
				}

				setting.float_settings[int(ProductSettingFloatMapper::SENSITIVITY)] = parametersTree.get<float>("SENSITIVITY");
				setting.float_settings[int(ProductSettingFloatMapper::MODEL_WIDTH)] = parametersTree.get<float>("MODEL_WIDTH");
				setting.float_settings[int(ProductSettingFloatMapper::MODEL_HEIGHT)] = parametersTree.get<float>("MODEL_HEIGHT");
				setting.float_settings[int(ProductSettingFloatMapper::MODEL_CATEGORY)] = parametersTree.get<float>("MODEL_CATEGORY");
			}

			if (group == "detect" && function == "signal")//第一次拍照尺寸
			{
				if (!pt.get_child_optional("parameters"))
				{
					LogERROR << "find model params failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				ptree parametersTree = pt.get_child("parameters");
				if (!parametersTree.get_child_optional("TEST_TYPE") || !parametersTree.get_child_optional("TEST_CHANNEL"))
				{
					LogERROR << "find TEST_TYPE, TEST_CHANNEL";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find TEST_TYPE, TEST_CHANNEL in parameters failed");
					return 1;
				}
				setting.float_settings[int(ProductSettingFloatMapper::TEST_TYPE)] = parametersTree.get<float>("TEST_TYPE");
				setting.float_settings[int(ProductSettingFloatMapper::TEST_CHANNEL)] = parametersTree.get<float>("TEST_CHANNEL");
			}

			if (group == "detect" && function == "defect")
			{
				if (!pt.get_child_optional("parameters"))
				{
					LogERROR << "find model params failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				ptree parametersTree = pt.get_child("parameters");

				for (size_t i = 0; i < 12; i++)
				{
					if(!parametersTree.get_child_optional("DEFECT_"+to_string(i)))
					{
						LogERROR << "find DEFECT_CUCAO, DEFECT_HEIDIAN, DEFECT_JIAFEN, DEFECT_JIETOU DEFECT_KELI DEFECT_LOULVBO DEFECT_LVBOPOSUN DEFECT_POSUN in parameters failed";
						response->write(SimpleWeb::StatusCode::client_error_bad_request, "find DEFECT_CUCAO, DEFECT_HEIDIAN, DEFECT_JIAFEN, DEFECT_JIETOU DEFECT_KELI DEFECT_LOULVBO DEFECT_LVBOPOSUN in parameters failed");
						return 1;
					}
					setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CUCAO)+i] = parametersTree.get<float>("DEFECT_"+to_string(i));
				}
				



				
				// if (!parametersTree.get_child_optional("DEFECT_0") || !parametersTree.get_child_optional("DEFECT_1")|| 
				// 	!parametersTree.get_child_optional("DEFECT_2") || 
				// 	!parametersTree.get_child_optional("DEFECT_4") || !parametersTree.get_child_optional("DEFECT_5")|| 
				// 	!parametersTree.get_child_optional("DEFECT_6") || !parametersTree.get_child_optional("DEFECT_7")|| 
				// 	!parametersTree.get_child_optional("DEFECT_8") || !parametersTree.get_child_optional("DEFECT_9")|| 
				// 	!parametersTree.get_child_optional("DEFECT_10"))
				// {
				// 	LogERROR << "find DEFECT_CUCAO, DEFECT_HEIDIAN, DEFECT_JIAFEN, DEFECT_JIETOU DEFECT_KELI DEFECT_LOULVBO DEFECT_LVBOPOSUN DEFECT_POSUN in parameters failed";
				// 	response->write(SimpleWeb::StatusCode::client_error_bad_request, "find DEFECT_CUCAO, DEFECT_HEIDIAN, DEFECT_JIAFEN, DEFECT_JIETOU DEFECT_KELI DEFECT_LOULVBO DEFECT_LVBOPOSUN in parameters failed");
				// 	return 1;
				// }
				// cout<<parametersTree.get<float>("DEFECT_10")<<" =================="<<endl;
				
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CUCAO)] = parametersTree.get<float>("DEFECT_0");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_HEIDIAN)] = parametersTree.get<float>("DEFECT_1");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_JIAFEN)] = parametersTree.get<float>("DEFECT_2");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_JIETOU)] = parametersTree.get<float>("DEFECT_3");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_KELI)] = parametersTree.get<float>("DEFECT_4");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LOULVBO)] = parametersTree.get<float>("DEFECT_5");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LVBOPOSUN)] = parametersTree.get<float>("DEFECT_6");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_POSUN)] = parametersTree.get<float>("DEFECT_7");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_QUEJIAO)] = parametersTree.get<float>("DEFECT_8");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_REFENGBULIANG)] = parametersTree.get<float>("DEFECT_9");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_YIWU)] = parametersTree.get<float>("DEFECT_10");
				// setting.float_settings[int(ProductSettingFloatMapper::DEFECT_JIAONANGDINGMAO)] = parametersTree.get<float>("DEFECT_11");
			}

			if (group == "detect" && function == "jiaonang")//第一次拍照尺寸
			{
				if (!pt.get_child_optional("enable"))
				{
					LogERROR << "find model params failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				setting.float_settings[int(ProductSettingFloatMapper::USE_JIAONANG)] = pt.get<float>("enable");
			}
			if (group == "detect" && function == "usepihao")//第一次拍照尺寸
			{
				if (!pt.get_child_optional("enable"))
				{
					LogERROR << "find model params failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				setting.float_settings[int(ProductSettingFloatMapper::USE_PIHAO)] = pt.get<float>("enable");
			}

			if (group == "detect" && function == "pngconvert")//第一次拍照尺寸
			{
				if (!pt.get_child_optional("enable"))
				{
					LogERROR << "find model params failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				setting.float_settings[int(ProductSettingFloatMapper::PNG_CONVERT)] = pt.get<float>("enable");
			}

			if (group == "detect" && function == "usemodel")//第一次拍照尺寸
			{
				if (!pt.get_child_optional("enable"))
				{
					LogERROR << "find model params failed";
					response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
					return 1;
				}

				setting.float_settings[int(ProductSettingFloatMapper::USE_MODEL)] = pt.get<float>("enable");
			}
			///////////////////////////////////////////
			// if (group == "detect" && function == "sensitivity")
			// {
			// 	if (!pt.get_child_optional("parameters"))
			// 	{
			// 		LogERROR << "find model params failed";
			// 		response->write(SimpleWeb::StatusCode::client_error_bad_request, "find dimension failed");
			// 		return 1;
			// 	}

			// 	ptree parametersTree = pt.get_child("parameters");
			// 	if (!parametersTree.get_child_optional("DEFECT_GOOD") || !parametersTree.get_child_optional("DEFECT_ZHEZHOU")|| 
			// 		!parametersTree.get_child_optional("DEFECT_PIHAO") || !parametersTree.get_child_optional("DEFECT_KONGLI")|| 
			// 		!parametersTree.get_child_optional("DEFECT_BANLI") || !parametersTree.get_child_optional("DEFECT_JUPI")|| 
			// 		!parametersTree.get_child_optional("DEFECT_CUOWEI") || !parametersTree.get_child_optional("DEFECT_CXBL")|| 
			// 		!parametersTree.get_child_optional("DEFECT_MAOFA") || !parametersTree.get_child_optional("DEFECT_LIEWEN")|| 
			// 		!parametersTree.get_child_optional("DEFECT_SHUANGP") ||!parametersTree.get_child_optional("DEFECT_LOUFEN"))
			// 	{
			// 		LogERROR << "find DEFECT_GOOD, DEFECT_ZHEZHOU, DEFECT_PIHAO, DEFECT_KONGLI DEFECT_BANLI DEFECT_JUPI DEFECT_CUOWEI DEFECT_CXBL DEFECT_MAOFA DEFECT_LIEWEN DEFECT_LOUFEN DEFECT_SHUANGP in parameters failed";
			// 		response->write(SimpleWeb::StatusCode::client_error_bad_request, "find DEFECT_GOOD, DEFECT_ZHEZHOU, DEFECT_PIHAO, DEFECT_KONGLI DEFECT_BANLI DEFECT_JUPI DEFECT_CUOWEI DEFECT_CXBL DEFECT_MAOFA DEFECT_LIEWEN DEFECT_LOUFEN DEFECT_SHUANGP in parameters failed");
			// 		return 1;
			// 	}
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_GOOD)] = parametersTree.get<float>("DEFECT_GOOD");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_ZHEZHOU)] = parametersTree.get<float>("DEFECT_ZHEZHOU");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_PIHAO)] = parametersTree.get<float>("DEFECT_PIHAO");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_KONGLI)] = parametersTree.get<float>("DEFECT_KONGLI");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_BANLI)] = parametersTree.get<float>("DEFECT_BANLI");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_JUPI)] = parametersTree.get<float>("DEFECT_JUPI");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CUOWEI)] = parametersTree.get<float>("DEFECT_CUOWEI");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_CXBL)] = parametersTree.get<float>("DEFECT_CXBL");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_MAOFA)] = parametersTree.get<float>("DEFECT_MAOFA");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LIEWEN)] = parametersTree.get<float>("DEFECT_LIEWEN");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_SHUANGP)] = parametersTree.get<float>("DEFECT_SHUANGP");
			// 	setting.float_settings[int(ProductSettingFloatMapper::DEFECT_LOUFEN)] = parametersTree.get<float>("DEFECT_LOUFEN");
			// }
			///////////////////////////////////////////



			RunningInfo::instance().GetTestProductSetting().UpdateCurrentProductSetting(setting);
			response->write("Success");

		}
		catch (const exception &e)
		{
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}

		return 0;
	};

	return 0;
}


int AppWebServer::SetImagePath()
{
	/*
   POST: http://localhost:8080/set_image_path?image_path=imagePath
   Return: Success
 */
	m_server.resource["^/set_image_path"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		auto query_fields = request->parse_query_string();
		try
		{
			string image_path = SimpleWeb::getValue(query_fields, "image_path");
			RunningInfo::instance().GetTestProductSetting().setImageName(image_path);
			LogINFO << "Update product setting " << image_path << " success";
			response->write("Success");
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}
		return 0;
	};
	return 0;
}

int AppWebServer::GetImagePath()
{
	/*
   POST: http://localhost:8080/get_image_path
   Return: Success
 */
	m_server.resource["^/get_image_path"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		try
		{
			string imagePath = RunningInfo::instance().GetTestProductSetting().getImageName();
			LogINFO << "get image_path: " << imagePath;

			response->write(imagePath);
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}
		return 0;
	};

	return 0;
}

int AppWebServer::SettingWrite()
{
	/*
   POST: http://localhost:8080/setting_write?prod=Product03
   Return: Success
 */
	m_server.resource["^/setting_write"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		auto query_fields = request->parse_query_string();
		try
		{
			string prod = SimpleWeb::getValue(query_fields, "prod");
			ptree pt;
			read_json(request->content, pt);

			ProductSetting::PRODUCT_SETTING setting = RunningInfo::instance().GetTestProductSetting().GetSettings();

			// use lambda function to release database resource before UpdateCurrentProductSetting, which open database again
			auto writeProdSetting = [](const string &prod, const ProductSetting::PRODUCT_SETTING &setting)
			{
				Database db;
				return db.prod_setting_write(prod, setting);
			};

			if (writeProdSetting(prod, setting))
			{
				LogERROR << "Update product setting " << prod << " in database failed";
				response->write(SimpleWeb::StatusCode::server_error_internal_server_error, "Update Product Setting " + prod + " Failed");
				return 1;
			}

			// update the memory each time setting is confirmed
			RunningInfo::instance().GetProductSetting().UpdateCurrentProductSetting(setting);
			LogINFO << "Update product setting " << prod << " success";
			response->write("Success");
			db_utils::ALARM("ModifyParameters");
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}
		return 0;
	};
	return 0;
}

int AppWebServer::GetPlcParams()
{
	m_server.resource["^/get_plc_params"]["GET"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		auto query_fields = request->parse_query_string();
		try
		{
			ProductSetting::PRODUCT_SETTING setting = RunningInfo::instance().GetTestProductSetting().GetSettings();
			boost::property_tree::ptree root;
			boost::property_tree::ptree plc_params;
			boost::property_tree::ptree speed(to_string(setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS)]));
			boost::property_tree::ptree dis_tri2cam(to_string(setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 1]));
			boost::property_tree::ptree dis_tri2purge(to_string(setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 2]));
			boost::property_tree::ptree purge_duration(to_string(setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 3]));
			boost::property_tree::ptree waiting_time(to_string(setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 4]));
			boost::property_tree::ptree dis_tri2printer(to_string(setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 5]));
			boost::property_tree::ptree dis_tri2printer_error(to_string(setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 6]));
			
			boost::property_tree::ptree speed_tip_txt("速度值范围: [1 - 80]");
			boost::property_tree::ptree tips;
			tips.put_child("speed", speed_tip_txt);
			root.put_child("tips", tips);
					
			plc_params.put_child("speed", speed);
			plc_params.put_child("dis_tri2cam", dis_tri2cam);
			plc_params.put_child("dis_tri2purge", dis_tri2purge);
			plc_params.put_child("purge_duration", purge_duration);
			plc_params.put_child("waiting_time", waiting_time);
			plc_params.put_child("dis_tri2printer", dis_tri2printer);
			plc_params.put_child("dis_tri2printer_error", dis_tri2printer_error);
			root.put_child("plc_params", plc_params);

            
			stringstream ss;
			write_json(ss, root);
			response->write(ss);
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
		}
	};
	return 0;
}


int AppWebServer::SetPlcParams()
{

	m_server.resource["^/set_plc_params"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		auto query_fields = request->parse_query_string();
		try
		{
			string prod = SimpleWeb::getValue(query_fields, "prod");
			ptree pt;
			read_json(request->content, pt);
			// load product setting, and other test related values
			if (pt.empty())
			{
				LogERROR << "Invalid content in test product setting";
				response->write(SimpleWeb::StatusCode::client_error_bad_request, "Invalid Content");
				return 1;
			}
			ProductSetting::PRODUCT_SETTING setting = RunningInfo::instance().GetTestProductSetting().GetSettings();

			if (!pt.get_child_optional("plc_params"))
			{
				LogERROR << "find plc_params failed";
				response->write(SimpleWeb::StatusCode::client_error_bad_request, "find plc_params failed");
				return 1;
			}
			ptree parametersTree = pt.get_child("plc_params");
			if (!parametersTree.get_child_optional("speed") || !parametersTree.get_child_optional("dis_tri2cam") ||
				!parametersTree.get_child_optional("dis_tri2purge") || !parametersTree.get_child_optional("purge_duration") ||
				!parametersTree.get_child_optional("waiting_time") || !parametersTree.get_child_optional("dis_tri2printer") ||
				!parametersTree.get_child_optional("dis_tri2printer_error"))
			{
				LogERROR << "find parameters in plc_params failed";
				response->write(SimpleWeb::StatusCode::client_error_bad_request, "find parameters in plc_params failed");
				return 1;
			}
			setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS)] = parametersTree.get<float>("speed");
			setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 1] = parametersTree.get<float>("dis_tri2cam");
			setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 2] = parametersTree.get<float>("dis_tri2purge");
			setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 3] = parametersTree.get<float>("purge_duration");
			setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 4] = parametersTree.get<float>("waiting_time");
			setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 5] = parametersTree.get<float>("dis_tri2printer");
			setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + 6] = parametersTree.get<float>("dis_tri2printer_error");

			int registersStart(0);
			
			LogINFO << "AppWebServer::SetPlcParams plc_params number: " << parametersTree.size();
			for (int i = 0; i < parametersTree.size(); i++)
			{
				// getIoManager()->write(0, i+1, setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + i]);
				float plc_data = setting.float_settings[int(ProductSettingFloatMapper::PLC_PARAMETERS) + i];
				unsigned short farray[2] = {0};
				*(float*)farray = plc_data;
				uint16_t write_regs[2];
				write_regs[1] = farray[0];
				write_regs[0] = farray[1];
				dynamic_pointer_cast<AppPlcManager>(getIoManager())->modbus_write_registers(registersStart+i*2, 2, write_regs);			
			}

			RunningInfo::instance().GetTestProductSetting().UpdateCurrentProductSetting(setting);
			auto writeProdSetting = [](const string &prod, const ProductSetting::PRODUCT_SETTING &setting)
			{
				Database db;
				return db.prod_setting_write(prod, setting);
			};
			if (writeProdSetting(prod, setting))
			{
				LogERROR << "Update product setting " << prod << " in database failed";
				response->write(SimpleWeb::StatusCode::server_error_internal_server_error, "Update Product Setting " + prod + " Failed");
				return 1;
			}

			// update the memory each time setting is confirmed
			RunningInfo::instance().GetProductSetting().UpdateCurrentProductSetting(setting);

			response->write("Success");
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}
		return 0;
	};
	return 0;
}

int AppWebServer::SetDefaultAutoParams()
{
	/*
   POST: http://localhost:8080/set_default_auto_params
   Return: Success
	*/
	m_server.resource["^/set_default_auto_params"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		try
		{
			response->write("Success");
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}
		return 0;
	};
	return 0;
}


int AppWebServer::Preview()
{

	m_server.resource["^/preview"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request)
	{
		auto query_fields = request->parse_query_string();
		try
		{
			string function = SimpleWeb::getValue(query_fields, "function");
			int phase = -1;
			switch (mapStringValues[function])
			{
			case StringValue::all:
				phase = 0;
				break;
			case StringValue::cropImage:
				phase = 1;
				break;
			case StringValue::rotationOfTD:
				phase = 2;
				break;
			default:
				break;
			}

			RunningInfo::instance().GetTestProductSetting().setPhase(phase);
			RunningInfo::instance().GetTestProductionInfo().SetTestStatus(1);
			int testStatus(1);
			while (testStatus != 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				testStatus = RunningInfo::instance().GetTestProductionInfo().GetTestStatus();
			}
            // LogINFO << "image size: " << m_image_pathes.size();
			// for (int i = 0; i < m_image_pathes.size(); i++)
			// {
			// 	RunningInfo::instance().GetTestProductSetting().setPhase(phase);
			// 	RunningInfo::instance().GetTestProductionInfo().SetTestStatus(2);
			// 	RunningInfo::instance().GetTestProductSetting().setImageName(m_image_pathes[i]);
			// 	/*
			// 	while ( RunningInfo::instance().GetTestProductionInfo().GetTestStatus() != 0)
			// 	{
			// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
			// 	}
			// 	*/
			// 	int testStatus(2);
			// 	while (testStatus != 0)
			// 	{
			// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
			// 		testStatus = RunningInfo::instance().GetTestProductionInfo().GetTestStatus();
			// 	}

			// }
			response->write("Success");
		}
		catch (const exception &e)
		{
			LogERROR << e.what();
			response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
			return 1;
		}
		return 0;
	};
	return 0;
}

