#include "switemdelegate.hpp"

#include <QPainter>
#include <QStyleOptionViewItem>

SWItemDelegate::SWItemDelegate(QObject *parent):
  QStyledItemDelegate(parent)
{}


QSize SWItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{

  auto size = QStyledItemDelegate::sizeHint(option, index);
  size.setHeight(20);
  return size;
}


void SWItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
						   const QModelIndex &index) const{

  //! Copiamos las opciones para poder modificarlas sin afectar al original
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);

  //! Detectamos si el mouse está pasando por encima de esta celda/fila
  if (opt.state & QStyle::State_MouseOver) {

	//! Definimos un color sutil para el hover
	QColor hoverColor = opt.palette.color(QPalette::Accent);
	hoverColor.setAlpha(100);

	//! Pintamos nuestro fondo con bordes redondeados
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setPen(Qt::NoPen);
	painter->setBrush(hoverColor);

	//! Dibujar rectángulo con bordes redondeados (radio de 3px)
	painter->drawRoundedRect(opt.rect, 3, 3);

	painter->restore();

	//! Eliminamos el flag de MouseOver para que la clase base no pinte encima
	opt.state &= ~QStyle::State_MouseOver;
  }

  //! Llamamos a la implementación base para que pinte el texto, iconos, bordes, etc.
  QStyledItemDelegate::paint(painter, opt, index);


}