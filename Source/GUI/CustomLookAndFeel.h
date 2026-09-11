#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace OAF
{

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff121318));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffff7700));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff9900));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2d36));
        setColour(juce::Label::textColourId, juce::Colour(0xffe0e4ec));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1e222a));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xff00f0ff));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3d4454));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f242e));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc5cad6));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto centre = bounds.getCentre();
        auto lineW = 4.5f;
        auto arcRadius = radius - lineW * 0.6f;

        // Background Track Arc
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff222630));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active Value Glow Track
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                   0.0f, rotaryStartAngle, toAngle, true);

            juce::Colour glowColour = slider.getName().containsIgnoreCase("gain") || slider.getName().containsIgnoreCase("drive") 
                                      ? juce::Colour(0xffff5500) 
                                      : juce::Colour(0xff00d4ff);

            // Subtle glow
            g.setColour(glowColour.withAlpha(0.25f));
            g.strokePath(valueArc, juce::PathStrokeType(lineW + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour(glowColour);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Inner Dial Body
        auto dialRadius = radius - lineW * 2.2f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a2e3a), centre.x, centre.y - dialRadius,
                                               juce::Colour(0xff161820), centre.x, centre.y + dialRadius, false));
        g.fillEllipse(centre.x - dialRadius, centre.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);

        // Border ring
        g.setColour(juce::Colour(0xff3c4354));
        g.drawEllipse(centre.x - dialRadius, centre.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f, 1.2f);

        // Indicator Pointer Dot / Notch
        juce::Path pointer;
        auto pointerLength = dialRadius * 0.75f;
        auto pointerThickness = 3.0f;
        pointer.addRoundedRectangle(-pointerThickness * 0.5f, -dialRadius + 2.0f, pointerThickness, pointerLength * 0.4f, 1.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));

        g.setColour(juce::Colour(0xffffffff));
        g.fillPath(pointer);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto cornerSize = 5.0f;

        if (button.getToggleState())
        {
            g.setColour(juce::Colour(0xffff6600));
            g.fillRoundedRectangle(bounds, cornerSize);
            g.setColour(juce::Colour(0xffff9933));
            g.drawRoundedRectangle(bounds, cornerSize, 1.5f);
        }
        else
        {
            auto baseCol = shouldDrawButtonAsDown ? juce::Colour(0xff151820)
                         : shouldDrawButtonAsHighlighted ? juce::Colour(0xff2d3342)
                         : juce::Colour(0xff1e232e);

            g.setColour(baseCol);
            g.fillRoundedRectangle(bounds, cornerSize);
            g.setColour(juce::Colour(0xff383e50));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        }
    }
};

} // namespace OAF
