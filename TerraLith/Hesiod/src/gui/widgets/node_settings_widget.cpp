/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "attributes.hpp"

#include "hesiod/app/hesiod_application.hpp"
#include "hesiod/gui/widgets/gui_utils.hpp"
#include "hesiod/gui/widgets/icon_check_box.hpp"
#include "hesiod/gui/widgets/node_attributes_widget.hpp"
#include "hesiod/gui/widgets/node_settings_widget.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/graph/graph_node.hpp"
#include "hesiod/model/utils.hpp"

namespace hesiod
{

NodeSettingsWidget::NodeSettingsWidget(QPointer<GraphNodeWidget> p_graph_node_widget,
                                       QWidget                  *parent)
    : QWidget(parent), p_graph_node_widget(p_graph_node_widget)
{
  Logger::log()->trace("NodeSettingsWidget::NodeSettingsWidget");

  if (!this->p_graph_node_widget)
    return;

  this->setMinimumWidth(300);

  this->setup_layout();
  this->setup_connections();
  this->update_content();
}

bool NodeSettingsWidget::is_auto_update_enabled() const
{
  return this->auto_update_checkbox && this->auto_update_checkbox->isChecked();
}

void NodeSettingsWidget::force_build()
{
  if (!this->p_graph_node_widget)
    return;

  this->p_graph_node_widget->force_build();
}

void NodeSettingsWidget::setup_connections()
{
  Logger::log()->trace("NodeSettingsWidget::setup_connections");

  if (!this->p_graph_node_widget)
    return;

  // GraphNodeWidget -> this (make sure the dialog is closed when
  // the graph is destroyed or if the node is deleted)

  // TODO fix, unsafe
  this->connect(this->p_graph_node_widget,
                &QObject::destroyed,
                this,
                [this]()
                {
                  this->p_graph_node_widget = nullptr;
                  this->deleteLater();
                });

  // connect selection changes -> update UI
  this->connect(this->p_graph_node_widget,
                &gngui::GraphViewer::selection_has_changed,
                this,
                &NodeSettingsWidget::update_content);

  this->connect(this->p_graph_node_widget,
                &GraphNodeWidget::update_finished,
                this,
                &NodeSettingsWidget::update_content);
}

void NodeSettingsWidget::setup_layout()
{
  Logger::log()->trace("NodeSettingsWidget::setup_layout");

  // --- TerraLith dark-theme stylesheet (cascades to all children) ---
  this->setStyleSheet(R"(
    /* === Root panel === */
    NodeSettingsWidget {
      background-color: #1E1E22;
      color: #E0E2E8;
    }

    /* === Scroll area === */
    QScrollArea {
      background-color: #1E1E22;
      border: none;
    }
    QScrollArea > QWidget > QWidget {
      background-color: #1E1E22;
    }
    QScrollBar:vertical {
      background: #1E1E22;
      width: 6px;
      margin: 0;
    }
    QScrollBar::handle:vertical {
      background: #333338;
      min-height: 24px;
      border-radius: 3px;
    }
    QScrollBar::handle:vertical:hover {
      background: #4A4A52;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
      height: 0;
    }

    /* === Input fields: dark inset look === */
    QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
      background-color: #151518;
      color: #E0E2E8;
      border: 1px solid #333338;
      border-radius: 4px;
      padding: 4px 6px;
      selection-background-color: #4396B2;
    }
    QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
      border: 1px solid #4396B2;
    }
    QComboBox::drop-down {
      border: none;
      background: transparent;
    }
    QComboBox QAbstractItemView {
      background-color: #1E1E22;
      color: #E0E2E8;
      border: 1px solid #333338;
      selection-background-color: #4396B2;
    }

    /* === Labels === */
    QLabel {
      color: #80838D;
      background: transparent;
    }

    /* === Sliders === */
    QSlider::groove:horizontal {
      height: 4px;
      background: #333338;
      border-radius: 2px;
    }
    QSlider::handle:horizontal {
      background: #4396B2;
      width: 12px;
      height: 12px;
      margin: -4px 0;
      border-radius: 6px;
    }
    QSlider::handle:horizontal:hover {
      background: #5AB0CC;
    }

    /* === Checkboxes === */
    QCheckBox {
      color: #E0E2E8;
      spacing: 6px;
      background: transparent;
    }
    QCheckBox::indicator {
      width: 16px;
      height: 16px;
      border: 1px solid #333338;
      border-radius: 3px;
      background: #151518;
    }
    QCheckBox::indicator:checked {
      background: #4396B2;
      border: 1px solid #4396B2;
    }

    /* === Push buttons === */
    QPushButton {
      background-color: #2A2A30;
      color: #E0E2E8;
      border: 1px solid #333338;
      border-radius: 4px;
      padding: 4px 12px;
    }
    QPushButton:hover {
      background-color: #333338;
      border: 1px solid #4396B2;
    }
    QPushButton:pressed {
      background-color: #4396B2;
    }

    /* === Tool buttons (toolbar icons) === */
    QToolButton {
      background: transparent;
      border: 1px solid transparent;
      border-radius: 4px;
      padding: 3px;
    }
    QToolButton:hover {
      background-color: #2A2A30;
      border: 1px solid #333338;
    }
    QToolButton:pressed {
      background-color: #4396B2;
    }

    /* === Group boxes (section headers) === */
    QGroupBox {
      background-color: #1E1E22;
      border: 1px solid #333338;
      border-radius: 6px;
      margin-top: 16px;
      padding-top: 20px;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      subcontrol-position: top left;
      padding: 4px 10px;
      background-color: #2A2A30;
      color: #E0E2E8;
      font-weight: bold;
      border: 1px solid #333338;
      border-radius: 4px;
    }

    /* === Separator lines === */
    QFrame[frameShape="4"] {
      color: #333338;
    }
  )");

  auto *layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  this->setLayout(layout);

  // --- Auto-Update / Force Build toolbar ---
  {
    auto *toolbar = new QWidget();
    toolbar->setStyleSheet(
        "background-color: #2A2A30;"
        "border-bottom: 2px solid #333338;");

    auto *tb_layout = new QHBoxLayout(toolbar);
    tb_layout->setContentsMargins(10, 6, 10, 6);
    tb_layout->setSpacing(8);

    this->auto_update_checkbox = new QCheckBox("Auto-Update");
    this->auto_update_checkbox->setChecked(true);
    this->auto_update_checkbox->setToolTip(
        "When enabled, changing node parameters automatically triggers graph computation.");
    tb_layout->addWidget(this->auto_update_checkbox);

    tb_layout->addStretch();

    this->force_build_button = new QPushButton("Force Build");
    this->force_build_button->setToolTip(
        "Manually trigger a full graph recomputation.");
    this->force_build_button->setEnabled(false); // disabled when auto-update is on
    tb_layout->addWidget(this->force_build_button);

    // Toggle: when auto-update is off, enable the Force Build button
    this->connect(this->auto_update_checkbox, &QCheckBox::toggled, this,
                  [this](bool checked)
                  { this->force_build_button->setEnabled(!checked); });

    // Force Build button triggers full graph update
    this->connect(this->force_build_button, &QPushButton::clicked, this,
                  &NodeSettingsWidget::force_build);

    layout->addWidget(toolbar);
  }

  // --- attributes widget inside scroll area

  {
    auto *container = new QWidget();
    container->setObjectName("settingsContainer");
    this->attr_layout = new QVBoxLayout(container);
    this->attr_layout->setContentsMargins(14, 14, 14, 14);
    this->attr_layout->setSpacing(10);

    auto *scroll = new QScrollArea();
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(container);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->addWidget(scroll);
  }
}

void NodeSettingsWidget::update_content()
{
  Logger::log()->trace("NodeSettingsWidget::update_content");

  if (!this->p_graph_node_widget)
    return;

  clear_layout(this->attr_layout);

  // lifetime safe getter
  GraphNode *p_gno = this->p_graph_node_widget->get_p_graph_node();
  if (!p_gno)
    return;

  // refill based on selected nodes (and pinned nodes)
  std::vector<std::string> selected_ids = this->p_graph_node_widget
                                              ->get_selected_node_ids();
  std::vector<std::string> all_ids = merge_unique(this->pinned_node_ids, selected_ids);

  for (auto &node_id : all_ids)
  {
    BaseNode *p_node = p_gno->get_node_ref_by_id<BaseNode>(node_id);
    if (!p_node)
    {
      remove_all_occurrences(this->pinned_node_ids, node_id);
      continue;
    }

    const QString node_caption = QString::fromStdString(p_node->get_caption());

    // --- Section header: styled container with pin button ---
    {
      auto *header = new QWidget();
      header->setStyleSheet(
          "background-color: #2A2A30;"
          "border-bottom: 2px solid #333338;"
          "border-radius: 4px;");

      auto *header_layout = new QHBoxLayout(header);
      header_layout->setContentsMargins(8, 6, 8, 6);
      header_layout->setSpacing(6);

      auto *button_pin = new IconCheckBox(this);
      button_pin->set_label(node_caption);
      button_pin->set_icons(HSD_ICON("push_pin"), HSD_ICON("push_pin_accent"));
      button_pin->setCheckable(true);
      button_pin->setChecked(contains(this->pinned_node_ids, node_id));
      header_layout->addWidget(button_pin);
      header_layout->addStretch();

      this->connect(button_pin,
                    &QCheckBox::toggled,
                    this,
                    [this, button_pin, node_id]
                    {
                      if (button_pin->isChecked())
                      {
                        if (!contains(this->pinned_node_ids, node_id))
                          this->pinned_node_ids.push_back(node_id);
                      }
                      else
                      {
                        remove_all_occurrences(this->pinned_node_ids, node_id);
                      }
                    });

      this->attr_layout->addWidget(header);
    }

    // GPU toggle checkbox for Vulkan-capable nodes
    if (p_node->supports_vulkan_compute())
    {
      auto *gpu_checkbox = new QCheckBox("Enable GPU Compute");
      // Read from the per-node "GPU" attribute when present (primary toggle);
      // fall back to vulkan_enabled_ for nodes that don't have one.
      bool gpu_on = p_node->is_vulkan_enabled();
      if (p_node->get_attributes_ref()->count("GPU"))
        gpu_on = p_node->get_attr<attr::BoolAttribute>("GPU");
      gpu_checkbox->setChecked(gpu_on);
      gpu_checkbox->setToolTip(
          "When enabled, this node uses Vulkan GPU acceleration.\n"
          "Disable to force CPU computation.");
      gpu_checkbox->setStyleSheet(
          "QCheckBox { color: #00FFAA; spacing: 6px; padding: 4px 8px; }"
          "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #333338; "
          "border-radius: 3px; background: #151518; }"
          "QCheckBox::indicator:checked { background: #00FFAA; border: 1px solid #00FFAA; }");

      this->connect(gpu_checkbox,
                    &QCheckBox::toggled,
                    this,
                    [this, p_node, node_id](bool checked)
                    {
                      p_node->set_vulkan_enabled(checked);

                      // trigger graph recompute
                      if (this->p_graph_node_widget)
                        this->p_graph_node_widget->on_node_reload_request(node_id);
                    });

      this->attr_layout->addWidget(gpu_checkbox);
    }

    auto *attr_widget = new NodeAttributesWidget(p_gno->get_shared(),
                                                 node_id,
                                                 this->p_graph_node_widget,
                                                 /* add_toolbar */ false,
                                                 /* parent */ nullptr);
    if (!attr_widget)
      continue;

    this->attr_layout->addWidget(attr_widget);
  }

  this->attr_layout->addStretch();
}

} // namespace hesiod
