#include "TransferModel.h"

#define COLUMN_COUNT 3

// Model

TransferModel::TransferModel()
{
	start=QIcon(":/resources/player_play.png");
	stop=QIcon(":/resources/player_pause.png");
	currentIndexSearch=0;
	haveSearchItem=false;
}

int TransferModel::columnCount( const QModelIndex& parent ) const
{
	return parent == QModelIndex() ? COLUMN_COUNT : 0;
}

QVariant TransferModel::data( const QModelIndex& index, int role ) const
{
	int row,column;
	row=index.row();
	column=index.column();
	if(index.parent()!=QModelIndex() || row < 0 || row >= transfertItemList.count() || column < 0 || column >= COLUMN_COUNT)
		return QVariant();

	const transfertItem& item = transfertItemList[row];
	if(role==Qt::UserRole)
		return item.id;
	else if(role==Qt::DisplayRole)
	{
		switch(column)
		{
			case 0:
				return item.source;
			break;
			case 1:
				return item.size;
			break;
			case 2:
				return item.destination;
			break;
			default:
			return QVariant();
		}
	}
	else if(role==Qt::DecorationRole)
	{
		switch(column)
		{
			case 0:
				if(stopId.contains(item.id))
					return stop;
				else if(startId.contains(item.id))
					return start;
				else
					return QVariant();
			break;
			default:
			return QVariant();
		}
	}
	else if(role==Qt::BackgroundRole)
	{
		if(!search_text.isEmpty() && (item.source.indexOf(search_text,0,Qt::CaseInsensitive)!=-1 || item.destination.indexOf(search_text,0,Qt::CaseInsensitive)!=-1))
		{
			if(haveSearchItem && searchId==item.id)
				return QColor(255,150,150,100);
			else
				return QColor(255,255,0,100);
		}
		else
			return QVariant();
	}
	return QVariant();
}

int TransferModel::rowCount( const QModelIndex& parent ) const
{
	return parent == QModelIndex() ? transfertItemList.count() : 0;
}

QVariant TransferModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < COLUMN_COUNT ) {
		switch ( section ) {
			case 0:
			return tr( "Source" );
			case 1:
			return tr( "Size" );
			case 2:
			return tr( "Target" );
		}
	}

	return QAbstractTableModel::headerData( section, orientation, role );
}

bool TransferModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
	row=index.row();
	column=index.column();
	if(index.parent()!=QModelIndex() || row < 0 || row >= transfertItemList.count() || column < 0 || column >= COLUMN_COUNT)
		return false;

	transfertItem& item = transfertItemList[row];
	if(role==Qt::UserRole)
	{
		item.id=value.toULongLong();
		return true;
	}
	else if(role==Qt::DisplayRole)
	{
		switch(column)
		{
			case 0:
				item.source=value.toString();
				emit dataChanged(index,index);
				return true;
			break;
			case 1:
				item.size=value.toString();
				emit dataChanged(index,index);
				return true;
			break;
			case 2:
				item.destination=value.toString();
				emit dataChanged(index,index);
				return true;
			break;
			default:
			return false;
		}
	}
	return false;
}

/*
  Return[0]: totalFile
  Return[1]: totalSize
  Return[2]: currentFile
  */
QList<quint64> TransferModel::synchronizeItems(const QList<returnActionOnCopyList>& returnActions)
{
	loop_size=returnActions.size();
	index_for_loop=0;
	totalFile=0;
	totalSize=0;
	currentFile=0;
	emit layoutAboutToBeChanged();
	while(index_for_loop<loop_size)
	{
		const returnActionOnCopyList& action=returnActions.at(index_for_loop);
		switch(action.type)
		{
			case AddingItem:
			{
				transfertItem newItem;
				newItem.id=action.addAction.id;
				newItem.source=action.addAction.sourceFullPath;
				newItem.size=facilityEngine->sizeToString(action.addAction.size);
				newItem.destination=action.addAction.destinationFullPath;
				transfertItemList<<newItem;
				totalFile++;
				totalSize+=action.addAction.size;
			}
			break;
			case MoveItem:
			{
				//bool current_entry=
				transfertItemList.move(action.userAction.position,action.userAction.moveAt);
			}
			break;
			case RemoveItem:
			{
				if(currentIndexSearch>0 && action.userAction.position<=currentIndexSearch)
					currentIndexSearch--;
				transfertItemList.removeAt(action.userAction.position);
				currentFile++;
				startId.removeOne(action.addAction.id);
				stopId.removeOne(action.addAction.id);
			}
			break;
			case PreOperation:
			{
				ItemOfCopyListWithMoreInformations tempItem;
				tempItem.currentProgression=0;
				tempItem.generalData=action.addAction;
				InternalRunningOperation << tempItem;
			}
			break;
			case Transfer:
			{
				if(!startId.contains(action.addAction.id))
					startId << action.addAction.id;
				stopId.removeOne(action.addAction.id);
				sub_index_for_loop=0;
				sub_loop_size=InternalRunningOperation.size();
				while(sub_index_for_loop<sub_loop_size)
				{
					if(InternalRunningOperation.at(sub_index_for_loop).generalData.id==action.addAction.id)
					{
						InternalRunningOperation[sub_index_for_loop].actionType=action.type;
						break;
					}
					sub_index_for_loop++;
				}
			}
			break;
			case PostOperation:
			{
				if(!stopId.contains(action.addAction.id))
					stopId << action.addAction.id;
				startId.removeOne(action.addAction.id);
				sub_index_for_loop=0;
				sub_loop_size=InternalRunningOperation.size();
				while(sub_index_for_loop<sub_loop_size)
				{
					if(InternalRunningOperation.at(sub_index_for_loop).generalData.id==action.addAction.id)
					{
						InternalRunningOperation.removeAt(sub_index_for_loop);
						break;
					}
					sub_index_for_loop++;
				}
			}
			break;
			case CustomOperation:
			{
				bool custom_with_progression=(action.addAction.size==1);
				//without progression
				if(custom_with_progression)
				{
					if(startId.removeOne(action.addAction.id))
						if(!stopId.contains(action.addAction.id))
							stopId << action.addAction.id;
				}
				//with progression
				else
				{
					stopId.removeOne(action.addAction.id);
					if(!startId.contains(action.addAction.id))
						startId << action.addAction.id;
				}
				sub_index_for_loop=0;
				sub_loop_size=InternalRunningOperation.size();
				while(sub_index_for_loop<sub_loop_size)
				{
					if(InternalRunningOperation.at(sub_index_for_loop).generalData.id==action.addAction.id)
					{
						InternalRunningOperation[sub_index_for_loop].actionType=action.type;
						InternalRunningOperation[sub_index_for_loop].custom_with_progression=custom_with_progression;
						InternalRunningOperation[sub_index_for_loop].currentProgression=0;
						break;
					}
					sub_index_for_loop++;
				}
			}
			break;
			default:
				//unknow code, ignore it
			break;
		}
		index_for_loop++;
	}
	emit layoutChanged();
	return QList<quint64>() << totalFile << totalSize << currentFile;
}

void TransferModel::setFacilityEngine(FacilityInterface * facilityEngine)
{
	this->facilityEngine=facilityEngine;
}

int TransferModel::search(const QString &text,bool searchNext)
{
	emit layoutAboutToBeChanged();
	search_text=text;
	emit layoutChanged();
	if(transfertItemList.size()==0)
		return -1;
	if(text.isEmpty())
		return -1;
	if(searchNext)
	{
		currentIndexSearch++;
		if(currentIndexSearch>=loop_size)
			currentIndexSearch=0;
	}
	index_for_loop=0;
	loop_size=transfertItemList.size();
	while(index_for_loop<loop_size)
	{
		if(transfertItemList.at(currentIndexSearch).source.indexOf(search_text,0,Qt::CaseInsensitive)!=-1 || transfertItemList.at(currentIndexSearch).destination.indexOf(search_text,0,Qt::CaseInsensitive)!=-1)
		{
			haveSearchItem=true;
			searchId=transfertItemList.at(currentIndexSearch).id;
			return currentIndexSearch;
		}
		currentIndexSearch++;
		if(currentIndexSearch>=loop_size)
			currentIndexSearch=0;
		index_for_loop++;
	}
	haveSearchItem=false;
	return -1;
}

int TransferModel::searchPrev(const QString &text)
{
	emit layoutAboutToBeChanged();
	search_text=text;
	emit layoutChanged();
	if(transfertItemList.size()==0)
		return -1;
	if(text.isEmpty())
		return -1;
	if(currentIndexSearch==0)
		currentIndexSearch=loop_size-1;
	else
		currentIndexSearch--;
	index_for_loop=0;
	loop_size=transfertItemList.size();
	while(index_for_loop<loop_size)
	{
		if(transfertItemList.at(currentIndexSearch).source.indexOf(search_text,0,Qt::CaseInsensitive)!=-1 || transfertItemList.at(currentIndexSearch).destination.indexOf(search_text,0,Qt::CaseInsensitive)!=-1)
		{
			haveSearchItem=true;
			searchId=transfertItemList.at(currentIndexSearch).id;
			return currentIndexSearch;
		}
		if(currentIndexSearch==0)
			currentIndexSearch=loop_size-1;
		else
			currentIndexSearch--;
		index_for_loop++;
	}
	haveSearchItem=false;
	return -1;
}

void TransferModel::setFileProgression(const QList<ProgressionItem> &progressionList)
{
	loop_size=InternalRunningOperation.size();
	sub_loop_size=progressionList.size();
	index_for_loop=0;
	while(index_for_loop<loop_size)
	{
		sub_index_for_loop=0;
		while(sub_index_for_loop<sub_loop_size)
		{
			if(progressionList.at(sub_index_for_loop).id==InternalRunningOperation.at(index_for_loop).generalData.id)
			{
				InternalRunningOperation[index_for_loop].generalData.size=progressionList.at(sub_index_for_loop).total;
				InternalRunningOperation[index_for_loop].currentProgression=progressionList.at(sub_index_for_loop).current;
				break;
			}
			sub_index_for_loop++;
		}
		index_for_loop++;
	}
}

TransferModel::currentTransfertItem TransferModel::getCurrentTransfertItem()
{
	currentTransfertItem returnItem;
	returnItem.haveItem=InternalRunningOperation.size()>0;
	if(returnItem.haveItem)
	{
		const ItemOfCopyListWithMoreInformations &itemTransfer=InternalRunningOperation.first();
		returnItem.from=itemTransfer.generalData.sourceFullPath;
		returnItem.to=itemTransfer.generalData.destinationFullPath;
		returnItem.current_file=itemTransfer.generalData.destinationFileName+", "+facilityEngine->sizeToString(itemTransfer.generalData.size);
		if(itemTransfer.generalData.size>0)
			returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
		else
			returnItem.progressBar_file=0;
	}
	return returnItem;
}
