#include "plugin.hpp"
#include <string>

#define MAX_INT32 0x7FFFFFFF
#define _5_OVER_MAX_INT32 2.32830643762289846205e-9f
#define NORMALIZE_10VPP(x) (x*0.2f)

#define _HP 5.08f
#define _3U 128.5f

struct Calvario : Module {
	enum ParamId {
		PARAM_MODE_SWITCH,
		PARAM_GAIN_IN1,
		PARAM_GAIN_IN2,
		PARAM_MIX_OUT,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT_IN1,
		SIGNAL_INPUT_IN2,
		SIGNAL_GAIN_MOD_IN1,
		SIGNAL_GAIN_MOD_IN2,
        SIGNAL_MIX_MOD_OUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		BLINK_LIGHT,
		LIGHTS_LEN
	};

	Calvario() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PARAM_GAIN_IN1, 0.f, 1.f, 0.f, "Gain - Input1");
		configParam(PARAM_GAIN_IN2, 0.f, 1.f, 0.f, "Gain - Input2");
		configParam(PARAM_MIX_OUT, 0.f, 1.f, 0.5f, "Mix");

		configInput(SIGNAL_GAIN_MOD_IN1, "CV Gain - Input1");
		configInput(SIGNAL_GAIN_MOD_IN2, "CV Gain - Input2");
        configInput(SIGNAL_MIX_MOD_OUT, "CV Mix - Output");

		configInput(SIGNAL_INPUT_IN1, "Signal - Input1");
		configInput(SIGNAL_INPUT_IN2, "Signal - Input2");
		configOutput(SIGNAL_OUTPUT, "Signal - Output");

		configSwitch(PARAM_MODE_SWITCH, 0.f, 1.f, 0.f, "Mode", {"Soft", "Hard"});
	}

	void process(const ProcessArgs& args) override {
	    
		// Input gain is ((Knob + normalized CV) * mode_gain)
		float gain1 = (params[PARAM_GAIN_IN1].getValue() + NORMALIZE_10VPP(math::clamp(inputs[SIGNAL_GAIN_MOD_IN1].getVoltage(), -5.f, 5.f))) * 0.2f;
		float gain2 = (params[PARAM_GAIN_IN2].getValue() + NORMALIZE_10VPP(math::clamp(inputs[SIGNAL_GAIN_MOD_IN2].getVoltage(), -5.f, 5.f))) * 0.2f;
		float cv_mix = NORMALIZE_10VPP(math::clamp(inputs[SIGNAL_MIX_MOD_OUT].getVoltage()));
        
		// Input signals
		float in_signal1 = inputs[SIGNAL_INPUT_IN1].getVoltage();
		float in_signal2 = inputs[SIGNAL_INPUT_IN2].getVoltage();
		
		// Save dry signal 1
		float dry_signal1 = in_signal1;
		
		// Limiter @0dB (10Vpp) => Normalize 10 Vpp to [-1.0 ; 1.0]
	    in_signal1 = NORMALIZE_10VPP(math::clamp(in_signal1, -5.f, 5.f) );
	    in_signal2 = NORMALIZE_10VPP(math::clamp(in_signal2, -5.f, 5.f) );
		
		// Apply gains
		in_signal1 *= gain1;
		in_signal2 *= gain2;

	    // Convert to int
	    int in1_toXor = (int) ((MAX_INT32) * in_signal1);
        int in2_toXor = (int) ((MAX_INT32) * in_signal2);

        // Apply XOR
        int xor_result = (in1_toXor ^ in2_toXor);
        // Bitshift for extra crispiness
        xor_result <<= (int(params[PARAM_MODE_SWITCH].getValue() * 2) + 2);

        // Convert to float and output gain staging
        float xor_toOut = ((float)xor_result * _5_OVER_MAX_INT32);
		
        // Mix (IN1 + XOR) 
		float mix = math::clamp(params[PARAM_MIX_OUT].getValue() + cv_mix, 0.f, 1.f);
        float output = (dry_signal1*(1.0f - mix) + xor_toOut*mix);
		
		// Apply Limiter @0dB (10 Vpp)
		output = math::clamp(output, -5.f, 5.f);
        outputs[SIGNAL_OUTPUT].setVoltage(output);

        // Blink light
        lights[BLINK_LIGHT].setBrightness(abs(output / 5.f));
	}
};

struct CalvarioWidget : ModuleWidget {
		CalvarioWidget(Calvario* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/calvario.svg")));

		// Screws
		//addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Input 1 Group
		addParam(createParamCentered<Trimpot>(mm2px(Vec(_HP, 11)), module, Calvario::PARAM_GAIN_IN1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 19)), module, Calvario::SIGNAL_GAIN_MOD_IN1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 29)), module, Calvario::SIGNAL_INPUT_IN1));
		
		// Input 2 Group
		addParam(createParamCentered<Trimpot>(mm2px(Vec(_HP, 41)), module, Calvario::PARAM_GAIN_IN2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 49)), module, Calvario::SIGNAL_GAIN_MOD_IN2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 59)), module, Calvario::SIGNAL_INPUT_IN2));
		
		// Mode Switch
		addParam(createParamCentered<CKSS>(mm2px(Vec(_HP, 71)), module, Calvario::PARAM_MODE_SWITCH));

		// Output Group
		addParam(createParamCentered<Trimpot>(mm2px(Vec(_HP, 81)), module, Calvario::PARAM_MIX_OUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 88)), module, Calvario::SIGNAL_MIX_MOD_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(_HP, 98)), module, Calvario::SIGNAL_OUTPUT));
		
		// LED
		addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(_HP, .9*_3U+2.4)), module, Calvario::BLINK_LIGHT));
	}
};


Model* modelCalvario = createModel<Calvario, CalvarioWidget>("Calvario");
