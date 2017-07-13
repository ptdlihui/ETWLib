#include "test.h"
#include <iostream>
#include <Qtcore/QCoreApplication>



Test::Test(QWidget *parent) : QWidget(parent),session(NULL),param(NULL)
{
	allProvidersName = ETWLib::GetUserProvidersName();
	grid = new QGridLayout;
	QGridLayout *gridCom = new QGridLayout;
	gridCom->addWidget(CreatStartButton(), 0, 0);
	gridCom->addWidget(CreatEndButton(), 0, 1);
	gridCom->addWidget(CreatSavePathButton(), 0, 2);
	gridCom->addWidget(CreatSelectAllCheckBox(), 0, 3);
	grid->addLayout(gridCom, 0, 0);
	grid->addWidget(CreatFilterLineEdit(), 0, 1);
	scrollAreaAllProvider = new QScrollArea;
	scrollAreaAllProvider->setWidget(CreatProvidesGroupBox());
	grid->addWidget(scrollAreaAllProvider, 1, 0);
	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	this->setLayout(grid);
	this->setWindowTitle(tr("Test Version"));
	const wchar_t* privilege[1] = {SE_SYSTEM_PROFILE_NAME};
	bool tokenValid = ETWLib::GrantPrivilegeW(privilege, 1);
	filePath = L"Test.etl";//if path not set,default path "Workspace\Test.etl" ,
	this->resize(480, 320);
}

QPushButton* Test::CreatStartButton() 
{
	start = new QPushButton(tr("Start"));
	start->setCheckable(true);
	start->setEnabled(true);
	connect(start, SIGNAL(clicked()), this, SLOT(HandleStart()));
	return start;
}

QPushButton* Test::CreatEndButton()
{
	end = new QPushButton(tr("End"));
	end->setCheckable(true);
	end->setEnabled(false);
	connect(end, SIGNAL(clicked()), this, SLOT(HandleEnd()));	
	return end;
}

QPushButton* Test::CreatSavePathButton() 
{
	save = new QPushButton(tr("savePath"));
	save->setCheckable(true);
	save->setEnabled(true);
	connect(save, SIGNAL(clicked()), this, SLOT(HandleSave()));
	return save;
}

QCheckBox* Test::CreatSelectAllCheckBox()
{
	selcetAll = new QCheckBox(tr("SelectAll"));
	selcetAll->setCheckable(true);
	selcetAll->setChecked(false);
	connect(selcetAll, SIGNAL(stateChanged(int)), this, SLOT(SeclectAllProviders(int)));
	return selcetAll;
}

QGroupBox* Test::CreatProvidesGroupBox()
{
	groupBoxAllProviders = new QGroupBox(tr("All Providers"));
	groupBoxAllProviders->setFlat(true);
	vBoxAllProviders = new QVBoxLayout;
	for (int i = 0; i < allProvidersName.size(); i++)
	{
		const QString qprovider_str = QString::fromStdWString(allProvidersName[i]);
		QCheckBox *checkBox = new QCheckBox(qprovider_str);
		vecAllProviders.push_back(checkBox);
		checkBox->setCheckState(Qt::Unchecked);
		connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(CheckBoxClicked(int)));
		vBoxAllProviders->addWidget(checkBox);
	}
	vBoxAllProviders->addStretch(1);
	groupBoxAllProviders->setLayout(vBoxAllProviders);
	return groupBoxAllProviders;
}

QLineEdit* Test::CreatFilterLineEdit()
{
	filter = new QLineEdit;
	filter->setPlaceholderText("Search here!");
	connect(filter, SIGNAL(textEdited(const QString)), this, SLOT(HandleFilter()));
	return filter;
}

void Test::HandleStart() 
{
	if (session == NULL) 
	{
		QMessageBox::information(this, tr("Hint"), tr("Set path first"));
		return;
	}
	ULONG status = session->StartSession(ETWLib::LogFileMode);
	start->setEnabled(false);
	end->setEnabled(true);
	if (status == 0) 
	{
		QMessageBox::information(this, tr("Error"), tr("Start failed"));
	}
}

void Test::HandleEnd() 
{
	session->CloseSession();
	delete session;
	delete param;
	session = nullptr;
	param = nullptr;
	start->setEnabled(true);
	end->setEnabled(false);
}

void Test::HandleFilter()
{
	vecAllFilterProviders.clear();
	QGroupBox groupbox(tr("searching result after filter"));
	groupbox.setFlat(true);
	QObject* sender = QObject::sender();
	QString filterqstr = ((QLineEdit*)sender)->text();
	QVBoxLayout vbox;
	QCheckBox selcetFilterAll("SelectFiltedAll");
	connect(&selcetFilterAll, SIGNAL(stateChanged(int)), this, SLOT(SeclectAllFiltedProviders(int)));
	vbox.addWidget(&selcetFilterAll);
	scrollAreaAllProvider->hide();

	QScrollArea* filterLeftProvidersScroll = new QScrollArea;
	QVBoxLayout* filterLeftlayout = new QVBoxLayout;
	QGroupBox* filterLeftBox = new QGroupBox;
	for (int i = 0; i < allProvidersName.size(); ++i) 
	{
		QString providerqstr = QString::fromStdWString(allProvidersName[i]);
		if (providerqstr.contains(filterqstr, Qt::CaseInsensitive)) 
		{
			vbox->addWidget(vecAllProviders[i]);
			vecAllFilterProviders.push_back(vecAllProviders[i]);
		}
		else 
		{			
			filterLeftlayout->addWidget(vecAllProviders[i]);
		}
	}
	vbox->addStretch(1);
	groupbox->setLayout(vbox);
	QScrollArea *scrollareafilter = new QScrollArea;
	scrollareafilter->setWidget(groupbox);
	grid->addWidget(scrollareafilter, 1, 1);

	filterLeftBox->setLayout(filterLeftlayout);
	filterLeftProvidersScroll->setWidget(filterLeftBox);
	grid->addWidget(filterLeftProvidersScroll, 1, 0);
}
		
void Test::HandleSave()
{	
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save path"), "", tr("*.etl"));
	QObject* sender = QObject::sender();
	((QPushButton*)sender)->setText(fileName);
	QMessageBox::information(this, tr("FileSaving"), tr("Path already set"));
	filePath = fileName.toStdWString();
	if (session == NULL) //Make sure there is only one instance exist
	{
		session = new ETWLib::ETWSession(L"TraceTest", filePath);
		param = new ETWLib::SessionParameters();
	}
	param->EnableProfilling(true);
	session->SetParameters(*param);
}

void Test::CheckBoxClicked(int state) 
{
	QObject* sender = QObject::sender();
	std::wstring wstr = ((QCheckBox*)sender)->text().toStdWString();
	if(state == Qt::Checked)
	{
		param->AddUserModeProvider(wstr, true);
	}
	else
	{
		param->EraseUserModeProvider(wstr);
	}	
}

void Test::SeclectAllProviders(int state) 
{	
	QObject* sender = QObject::sender();
	std::wstring wstr = ((QCheckBox*)sender)->text().toStdWString();
	if (state == Qt::Checked)
	{
		for (int i = 0; i < vecAllProviders.size(); ++i)
		{
			vecAllProviders[i]->setCheckState(Qt::Checked);
		}
	}
	else
	{
		for (int i = 0; i < vecAllProviders.size(); ++i)
		{
			vecAllProviders[i]->setCheckState(Qt::Unchecked);
		}
	}	
}

void Test::SeclectAllFiltedProviders(int state) 
{
	QObject* sender = QObject::sender();
	std::wstring wstr = ((QCheckBox*)sender)->text().toStdWString();
	if (state == Qt::Checked)
	{
		for (int i = 0; i < vecAllFilterProviders.size(); ++i)
		{
			vecAllFilterProviders[i]->setCheckState(Qt::Checked);
		}
	}
	else
	{
		for (int i = 0; i < vecAllFilterProviders.size(); ++i)
		{
			vecAllFilterProviders[i]->setCheckState(Qt::Unchecked);
		}
	}
}