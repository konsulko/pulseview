/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "header.h"
#include "view.h"

#include "signal.h"
#include "../sigsession.h"

#include <assert.h>

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>

#include <pv/widgets/popup.h>

using std::max;
using std::make_pair;
using std::pair;
using std::shared_ptr;
using std::vector;

namespace pv {
namespace view {

const int Header::Padding = 12;
const int Header::BaselineOffset = 5;

Header::Header(View &parent) :
	MarginWidget(parent),
	_dragging(false)
{
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);

	connect(&_view.session(), SIGNAL(signals_changed()),
		this, SLOT(on_signals_changed()));

	connect(&_view, SIGNAL(signals_moved()),
		this, SLOT(on_signals_moved()));

	// Trigger the initial event manually. The default device has signals
	// which were created before this object came into being
	on_signals_changed();
}

QSize Header::sizeHint() const
{
	int max_width = 0;

	const vector< shared_ptr<RowItem> > row_items(_view.child_items());
	for (shared_ptr<RowItem> r : row_items) {
		assert(r);

		if (r->enabled()) {
			max_width = max(max_width, (int)r->label_rect(0).width());
		}
	}

	return QSize(max_width + Padding + BaselineOffset, 0);
}

shared_ptr<RowItem> Header::get_mouse_over_row_item(const QPoint &pt)
{
	const int w = width() - BaselineOffset;
	const vector< shared_ptr<RowItem> > row_items(_view.child_items());

	for (const shared_ptr<RowItem> r : row_items)
	{
		assert(r);
		if (r->enabled() && r->label_rect(w).contains(pt))
			return r;
	}

	return shared_ptr<RowItem>();
}

void Header::clear_selection()
{
	const vector< shared_ptr<RowItem> > row_items(_view.child_items());
	for (const shared_ptr<RowItem> r : row_items) {
		assert(r);
		r->select(false);
	}

	update();
}

void Header::paintEvent(QPaintEvent*)
{
	// The trace labels are not drawn with the arrows exactly on the
	// left edge of the widget, because then the selection shadow
	// would be clipped away.
	const int w = width() - BaselineOffset;
	const vector< shared_ptr<RowItem> > row_items(_view.child_items());

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	const bool dragging = !_drag_row_items.empty();
	for (const shared_ptr<RowItem> r : row_items)
	{
		assert(r);

		const bool highlight = !dragging &&
			r->label_rect(w).contains(_mouse_point);
		r->paint_label(painter, w, highlight);
	}

	painter.end();
}

void Header::mousePressEvent(QMouseEvent *event)
{
	assert(event);

	const vector< shared_ptr<RowItem> > row_items(_view.child_items());

	if (event->button() & Qt::LeftButton) {
		_mouse_down_point = event->pos();

		// Save the offsets of any signals which will be dragged
		for (const shared_ptr<RowItem> r : row_items)
			if (r->selected())
				_drag_row_items.push_back(
					make_pair(r, r->v_offset()));
	}

	// Select the signal if it has been clicked
	const shared_ptr<RowItem> mouse_over_row_item =
		get_mouse_over_row_item(event->pos());
	if (mouse_over_row_item) {
		if (mouse_over_row_item->selected())
			mouse_over_row_item->select(false);
		else {
			mouse_over_row_item->select(true);

			if (~QApplication::keyboardModifiers() &
				Qt::ControlModifier)
				_drag_row_items.clear();

			// Add the signal to the drag list
			if (event->button() & Qt::LeftButton)
				_drag_row_items.push_back(
					make_pair(mouse_over_row_item,
					mouse_over_row_item->v_offset()));
		}
	}

	if (~QApplication::keyboardModifiers() & Qt::ControlModifier) {
		// Unselect all other signals because the Ctrl is not
		// pressed
		for (const shared_ptr<RowItem> r : row_items)
			if (r != mouse_over_row_item)
				r->select(false);
	}

	selection_changed();
	update();
}

void Header::mouseReleaseEvent(QMouseEvent *event)
{
	using pv::widgets::Popup;

	assert(event);
	if (event->button() == Qt::LeftButton) {
		if (_dragging)
			_view.normalize_layout();
		else
		{
			const shared_ptr<RowItem> mouse_over_row_item =
				get_mouse_over_row_item(event->pos());
			if (mouse_over_row_item) {
				const int w = width() - BaselineOffset;
				Popup *const p =
					mouse_over_row_item->create_popup(&_view);
				p->set_position(mapToGlobal(QPoint(w,
					mouse_over_row_item->get_y())),
					Popup::Right);
				p->show();
			}
		}

		_dragging = false;
		_drag_row_items.clear();
	}
}

void Header::mouseMoveEvent(QMouseEvent *event)
{
	assert(event);
	_mouse_point = event->pos();

	if (!(event->buttons() & Qt::LeftButton))
		return;

	if ((event->pos() - _mouse_down_point).manhattanLength() <
		QApplication::startDragDistance())
		return;

	// Move the signals if we are dragging
	if (!_drag_row_items.empty())
	{
		_dragging = true;

		const int delta = event->pos().y() - _mouse_down_point.y();

		for (auto i = _drag_row_items.begin();
			i != _drag_row_items.end(); i++) {
			const std::shared_ptr<RowItem> row_item((*i).first);
			if (row_item) {
				const int y = (*i).second + delta;
				const int y_snap =
					((y + View::SignalSnapGridSize / 2) /
						View::SignalSnapGridSize) *
						View::SignalSnapGridSize;
				row_item->set_v_offset(y_snap);

				// Ensure the trace is selected
				row_item->select();
			}
			
		}

		signals_moved();
	}

	update();
}

void Header::leaveEvent(QEvent*)
{
	_mouse_point = QPoint(-1, -1);
	update();
}

void Header::contextMenuEvent(QContextMenuEvent *event)
{
	const shared_ptr<RowItem> r = get_mouse_over_row_item(_mouse_point);

	if (r)
		r->create_context_menu(this)->exec(event->globalPos());
}

void Header::keyPressEvent(QKeyEvent *e)
{
	assert(e);

	switch (e->key())
	{
	case Qt::Key_Delete:
	{
		const vector< shared_ptr<RowItem> > row_items(_view.child_items());
		for (const shared_ptr<RowItem> r : row_items)
			if (r->selected())
				r->delete_pressed();
		break;
	}
	}
}

void Header::on_signals_changed()
{
	const vector< shared_ptr<RowItem> > row_items(_view.child_items());
	for (shared_ptr<RowItem> r : row_items) {
		assert(r);
		connect(r.get(), SIGNAL(visibility_changed()),
			this, SLOT(on_trace_changed()));
		connect(r.get(), SIGNAL(text_changed()),
			this, SLOT(on_trace_changed()));
		connect(r.get(), SIGNAL(colour_changed()),
			this, SLOT(update()));
	}
}

void Header::on_signals_moved()
{
	update();
}

void Header::on_trace_changed()
{
	update();
	geometry_updated();
}

} // namespace view
} // namespace pv
