/*

LICENSE


This program is free software; you can redistribute it 
and/or modify it under the terms of the GNU General Public 
License (GPL) as published by the Free Software Foundation; 
either version 2 of the License, or (at your option) any 
later version.

This program is distributed in the hope that it will be 
useful, but WITHOUT ANY WARRANTY; without even the 
implied warranty of MERCHANTABILITY or FITNESS FOR A 
PARTICULAR PURPOSE.  See the GNU General Public License 
for more details.

To read the license please visit
http://www.gnu.org/copyleft/gpl.html

*/

#include "mainwindowimpl.h"

AddressWindowImpl* MainWindowImpl::viewAddressesWindow(int record, bool locked) {
  AddressWindowImpl *addressWindow = new AddressWindowImpl(record,locked,addressTable,currencyTable);
  addressWindow->show();
  return addressWindow;
}//end viewAddressesWindow(int,bool)

AddressWindowImpl* MainWindowImpl::viewAddressesWindow() {
	return viewAddressesWindow(0,true);
}//end viewAddressesWindow

void MainWindowImpl::addAddresses() {
	AddressWindowImpl *addressWindow = viewAddressesWindow(0,false);
	addressWindow->newRecord();
}//end addAddresses

void MainWindowImpl::selectBoxActivated(int record) {
	if(record < 1) {//if invalid (ie, they picked the '- Contacts -' entry)
		return;
	}//end if record < 0
	viewAddressesWindow(record - 1,true);
}//end viewAddressesWindow(int)

void MainWindowImpl::closeEvent(QCloseEvent *event) {
	qApp->quit();
}//end closeEvent

void MainWindowImpl::f11() {
	QStringList tables = db.tables();
	QSqlTableModel *tableA[tables.size()];
	QTableView *views[tables.size()];
	for(int i = 0; i < tables.size(); i++) {
		tableA[i] = new QSqlTableModel();
		tableA[i]->setTable(tables.at(i));
		tableA[i]->select();
		views[i] = new QTableView;
		views[i]->setModel(tableA[i]);
		views[i]->show();
		views[i]->setWindowTitle(tables.at(i));
	}//end for i < tables.size()
}//end f11

void MainWindowImpl::showMailingLabels() {
  MailingLabelsImpl *widget = new MailingLabelsImpl(addressTable);
  widget->show();	
}//end printTest

void MainWindowImpl::enterEvent(QEvent *event) {
//	qDebug() << "MainWindowImpl::enterEvent"; //do not display because it plays havoc with --debug
	addressTable->select();
	while(addressTable->canFetchMore())
		addressTable->fetchMore();
	nameSelectBox->clear();
	QSqlRecord loopRecord;
	nameSelectBox->addItem("- Contacts -");
	for(int i = 0; i < addressTable->rowCount(); i++) {//loop through Addresses table
		loopRecord = addressTable->record(i);
		if(loopRecord.value("isOrganization").toString() == "1")
			nameSelectBox->addItem(loopRecord.value("organization").toString());
		else
			nameSelectBox->addItem(loopRecord.value("lastName").toString() + ", " + loopRecord.value("firstName").toString());
	}//end for i < addressTable->rowCount()
}//end enterEvent

void MainWindowImpl::showEvent(QEvent *event) {
	qDebug() << "MainWindowImpl::showEvent";
	QStringList args = qApp->arguments();
	int importIndex = args.indexOf(QRegExp("--import"));
	if(importIndex != -1) import(args.at(importIndex+1));
}//end showEvent

void MainWindowImpl::about() {
	QMessageBox::about(this, tr("About MIM"), tr("<b>Work in progress.</b><br>Moving from MSAccess to Qt.<br>Pre-release 1.1197759106<br><br><a href=\"http://mim.singpolyma.net/\">Project Website</a><br><a href=\"mailto:mim@singpolyma.net\">Email</a><br><br>Some icons from the <a href=\"http://famfamfam.com/lab/icons/silk/\">Silk icon set</a>"));
}//end about

void importFinish(QStringList map, QWidget* mainwindow, QSqlDatabase db) {
	qDebug() << "importFinish";
	MainWindowImpl* mw = (MainWindowImpl*)mainwindow;
	QSqlTableModel* addressTable = mw->getAddressTable();
	QSqlQuery query(db);
	if (!query.exec("select")) {
		qFatal("Unable to perform Query");
		return;
	}//end if ! query.exec
	QSqlRecord loopRecord;
	QSqlRecord newRecord = addressTable->record(0);
	while(query.next()) {
		newRecord.clearValues();
		loopRecord = ((SxWResult*)(query.result()))->record();
		QSqlField field, field2;
		for(int i = 0; i < loopRecord.count(); i++) {
			field = loopRecord.field(i);
			if(map.at(i) != "[none]") {
				field2 = newRecord.field(map.at(i));
				if(field2.type() == QVariant::Bool || field2.type() == QVariant::Int) {
					if(
						field.value().toString().toLower() == "true" ||
						field.value().toString().toLower() == "t"    ||
						field.value().toString().toLower() == "yes"  ||
						field.value().toString().toLower() == "y"
						) {
							newRecord.setValue(map.at(i), "1");
					} else if(
						field.value().toString().toLower() == "false" ||
						field.value().toString().toLower() == "f"     ||
						field.value().toString().toLower() == "no"    ||
						field.value().toString().toLower() == "n"
						) {
							newRecord.setValue(map.at(i), "0");
					} else {
						if(field2.type() == QVariant::Bool) newRecord.setValue(map.at(i), "0");
							else newRecord.setValue(map.at(i), field.value().toString());
					}//end if-elses
				} else newRecord.setValue(map.at(i), field.value().toString());
			}//end if != [none]
		}//end for
		addressTable->insertRecord(-1, newRecord);
	}//end while next
	addressTable->submitAll();
	db.close();
}//end importFinish

void MainWindowImpl::import() {
	qDebug("MainWindowImpl::import");
	QString documentsPath = QDir::homePath();
	if(QDir(QDir::homePath() + QDir::separator() + tr("Documents") + QDir::separator()).exists()) documentsPath = QDir::homePath() + QDir::separator() + tr("Documents") + QDir::separator();
	if(QDir(QDir::homePath() + QDir::separator() + tr("My Documents") + QDir::separator()).exists()) documentsPath = QDir::homePath() + QDir::separator() + tr("My Documents") + QDir::separator();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), documentsPath, tr("CSV Files (*.csv)"));
	import(fileName);
}//end import()

void MainWindowImpl::import(QString fileName) {
	qDebug("MainWindowImpl::import(%s)", fileName.toAscii().data());
	if(fileName == "") return;
	QSqlDatabase::registerSqlDriver( "SXWPLAIN", new QSqlDriverCreator<SxWPlainDriver> );
	QSqlDatabase db = QSqlDatabase::addDatabase("SXWPLAIN","import");
	db.setDatabaseName(fileName);
	if (!db.open()) {
		 qDebug("SQL Driver not found");
		 return;
	}//end if ! db.open
	QSqlQuery query(db);
	if (!query.exec("select")) {
		qDebug("Unable to perform Query");
		return;
	}//end if ! query.exec
	query.next();
	(new SqlImportWindow(this,((SxWResult*)(query.result()))->record(),addressTable->record(0),&importFinish,db))->show();
}//end import(fileName)
