/**
* This file was autogenerated by makekdewidgets. Any changes will be lost!
* The generated code in this file is licensed under the same license that the
* input file.
*/
#include <qwidgetplugin.h>
#include <QPixmap>

#include <KAboutData>
#include <addressbooklinkwidget.h>
#include <kopetelistview.h>
#include <kopetelistviewsearchline.h>
#ifndef EMBED_IMAGES
#include <kstandarddirs.h>
#endif

class KopeteWidgets : public QWidgetPlugin
{
public:
	KopeteWidgets();
	
	virtual ~KopeteWidgets();
	
	virtual QStringList keys() const
	{
		QStringList result;
		for (WidgetInfos::ConstIterator it = m_widgets.begin(); it != m_widgets.end(); ++it)
			result << it.key();
		return result;
	}
	
	virtual QWidget *create(const QString &key, QWidget *parent = 0, const char *name = 0);
	
	virtual QIcon iconSet(const QString &key) const
	{
#ifdef EMBED_IMAGES
		QPixmap pix(m_widgets[key].iconSet);
#else
		QPixmap pix(KStandardDirs::locate( "data", 
			QLatin1String("kopetewidgets/pics/") + m_widgets[key].iconSet));
#endif
		return QIcon(pix);
	}
	
	virtual bool isContainer(const QString &key) const { return m_widgets[key].isContainer; }
	
	virtual QString group(const QString &key) const { return m_widgets[key].group; }
	
	virtual QString includeFile(const QString &key) const { return m_widgets[key].includeFile; }
	
	virtual QString toolTip(const QString &key) const { return m_widgets[key].toolTip; }
	
	virtual QString whatsThis(const QString &key) const { return m_widgets[key].whatsThis; }
private:
	struct WidgetInfo
	{
		QString group;
#ifdef EMBED_IMAGES
		QPixmap iconSet;
#else
		QString iconSet;
#endif
		QString includeFile;
		QString toolTip;
		QString whatsThis;
		bool isContainer;
	};
	typedef QMap<QString, WidgetInfo> WidgetInfos;
	WidgetInfos m_widgets;
};
KopeteWidgets::KopeteWidgets()
{
        WidgetInfo widget;

	widget.group = QLatin1String("Input (Kopete)");
#ifdef EMBED_IMAGES
	widget.iconSet = QPixmap(kopete__ui__addressbooklinkwidget_xpm);
#else
	widget.iconSet = QLatin1String("kopete__ui__addressbooklinkwidget.png");
#endif
	widget.includeFile = QLatin1String("addressbooklinkwidget.h");
	widget.toolTip = QLatin1String("Address Book Link Widget (Kopete)");
	widget.whatsThis = QLatin1String("KContacts::Addressee display/selector");
	widget.isContainer = false;
	m_widgets.insert(QLatin1String("Kopete::UI::AddressBookLinkWidget"), widget);

	widget.group = QLatin1String("Views (Kopete)");
#ifdef EMBED_IMAGES
	widget.iconSet = QPixmap(kopete__ui__listview__listview_xpm);
#else
	widget.iconSet = QLatin1String("kopete__ui__listview__listview.png");
#endif
	widget.includeFile = QLatin1String("kopetelistview.h");
	widget.toolTip = QLatin1String("List View (Kopete)");
	widget.whatsThis = QLatin1String("A component capable list view widget.");
	widget.isContainer = false;
	m_widgets.insert(QLatin1String("Kopete::UI::ListView::ListView"), widget);

	widget.group = QLatin1String("Input (Kopete)");
#ifdef EMBED_IMAGES
	widget.iconSet = QPixmap(kopete__ui__listview__searchline_xpm);
#else
	widget.iconSet = QLatin1String("kopete__ui__listview__searchline.png");
#endif
	widget.includeFile = QLatin1String("kopetelistviewsearchline.h");
	widget.toolTip = QLatin1String("List View Search Line (Kopete)");
	widget.whatsThis = QLatin1String("Search line able to use Kopete custom list View.");
	widget.isContainer = false;
	m_widgets.insert(QLatin1String("Kopete::UI::ListView::SearchLine"), widget);

	KAboutData("kopetewidgets"); // if it's the only KAboutData object then it stays as
        // KGlobal::mainComponent()
}
KopeteWidgets::~KopeteWidgets()
{
	
}
QWidget *KopeteWidgets::create(const QString &key, QWidget *parent, const char *name)
{

         if (key == QLatin1String("Kopete::UI::AddressBookLinkWidget"))
                return new Kopete::UI::AddressBookLinkWidget(parent, name);

         if (key == QLatin1String("Kopete::UI::ListView::ListView"))
                return new Kopete::UI::ListView::ListView(parent, name);

         if (key == QLatin1String("Kopete::UI::ListView::SearchLine"))
                return new Kopete::UI::ListView::SearchLine(parent, 0, name);

	return 0;
}
KDE_Q_EXPORT_PLUGIN(KopeteWidgets)

