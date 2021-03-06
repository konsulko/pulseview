/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2013 Joel Holdsworth <joel@airwebreathe.org.uk>
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

#ifndef PULSEVIEW_PV_VIEW_HEADERITEM_H
#define PULSEVIEW_PV_VIEW_HEADERITEM_H

#include <memory>

#include <QPropertyAnimation>

#include "rowitempaintparams.hpp"
#include "selectableitem.hpp"

namespace pv {
namespace view {

class RowItemOwner;

class RowItem : public SelectableItem,
	public std::enable_shared_from_this<pv::view::RowItem>
{
	Q_OBJECT
	Q_PROPERTY(int visual_v_offset
		READ visual_v_offset
		WRITE set_visual_v_offset)

public:
	/**
	 * Constructor.
	 */
	RowItem();

	/**
	 * Gets the vertical layout offset of this signal.
	 */
	int layout_v_offset() const;

	/**
	 * Sets the vertical layout offset of this signal.
	 */
	void set_layout_v_offset(int v_offset);

	/**
	 * Gets the vertical visual offset of this signal.
	 */
	int visual_v_offset() const;

	/**
	 * Sets the vertical visual offset of this signal.
	 */
	void set_visual_v_offset(int v_offset);

	/**
	 * Sets the visual and layout offset of this signal.
	 */
	void force_to_v_offset(int v_offset);

	/**
	 * Begins an animation that will animate the visual offset toward
	 * the layout offset.
	 */
	void animate_to_layout_v_offset();

	/**
	 * Gets the owner this trace in the view trace hierachy.
	 */
	pv::view::RowItemOwner* owner() const;

	/**
	 * Sets the owner this trace in the view trace hierachy.
	 * @param The new owner of the trace.
	 */
	void set_owner(pv::view::RowItemOwner *owner);

	/**
	 * Gets the visual y-offset of the axis.
	 */
	int get_visual_y() const;

	/**
	 * Gets the drag point of the row item.
	 */
	QPoint point() const;

	/**
	 * Computes the vertical extents of the contents of this row item.
	 * @return A pair containing the minimum and maximum y-values.
	 */
	virtual std::pair<int, int> v_extents() const = 0;

	/**
	 * Paints the background layer of the trace with a QPainter
	 * @param p the QPainter to paint into.
	 * @param pp the painting parameters object to paint with.
	 **/
	virtual void paint_back(QPainter &p, const RowItemPaintParams &pp);

	/**
	 * Paints the mid-layer of the trace with a QPainter
	 * @param p the QPainter to paint into.
	 * @param pp the painting parameters object to paint with.
	 **/
	virtual void paint_mid(QPainter &p, const RowItemPaintParams &pp);

	/**
	 * Paints the foreground layer of the trace with a QPainter
	 * @param p the QPainter to paint into.
	 * @param pp the painting parameters object to paint with.
	 **/
	virtual void paint_fore(QPainter &p, const RowItemPaintParams &pp);

	/**
	 * Paints the signal label.
	 * @param p the QPainter to paint into.
	 * @param rect the rectangle of the header area.
	 * @param hover true if the label is being hovered over by the mouse.
	 */
	virtual void paint_label(QPainter &p, const QRect &rect, bool hover) = 0;

public:
	virtual void hover_point_changed();

protected:
	pv::view::RowItemOwner *owner_;

	int layout_v_offset_;
	int visual_v_offset_;

private:
	QPropertyAnimation v_offset_animation_;
};

} // namespace view
} // namespace pv

#endif // PULSEVIEW_PV_VIEW_HEADERITEM_H
