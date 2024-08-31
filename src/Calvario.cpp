#include "plugin.hpp"
#include <string>

#define MAX_INT32 0x7FFFFFFF
#define _HP 5.08
#define _3U 128.5

struct Calvario : Module {
	enum ParamId {
		GAIN_SWITCH,
		PARAM_GAIN_IN1,
		PARAM_GAIN_IN2,
		PARAM_GAIN_OUT,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT_IN1,
		SIGNAL_INPUT_IN2,
		SIGNAL_GAIN_MOD_IN1,
		SIGNAL_GAIN_MOD_IN2,
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
		configParam(PARAM_GAIN_IN1, -1.f, 1.f, 0.f, "Gain - Input1");
		configParam(PARAM_GAIN_IN2, -1.f, 1.f, 0.f, "Gain - Input2");
		configParam(PARAM_GAIN_OUT, 0.f, 1.f, 0.5f, "Gain - Output");

		configInput(SIGNAL_GAIN_MOD_IN1, "CV Gain - Input1");
		configInput(SIGNAL_GAIN_MOD_IN2, "CV Gain - Input2");

		configInput(SIGNAL_INPUT_IN1, "Signal - Input1");
		configInput(SIGNAL_INPUT_IN2, "Signal - Input2");
		configOutput(SIGNAL_OUTPUT, "Signal - Output");

		configSwitch(GAIN_SWITCH, 0.f, 1.f, 0.f, "Mode", {"Light", "Heavy"});
	}

	void process(const ProcessArgs& args) override {
	    // Input gain staging
	    float attenuate = 0.1f + (0.4f * params[GAIN_SWITCH].getValue());
	    
		float gain1 = (params[PARAM_GAIN_IN1].getValue() + math::clamp(inputs[SIGNAL_GAIN_MOD_IN1].getVoltage(), -5.f, 5.f) * 0.2f) * attenuate;
		float gain2 = (params[PARAM_GAIN_IN2].getValue() + math::clamp(inputs[SIGNAL_GAIN_MOD_IN2].getVoltage(), -5.f, 5.f) * 0.2f) * attenuate;
	    
	    float in1 = math::clamp(inputs[SIGNAL_INPUT_IN1].getVoltage(), -5.f, 5.f) * gain1;
	    float in2 = math::clamp(inputs[SIGNAL_INPUT_IN2].getVoltage(), -5.f, 5.f) * gain2; 

	    // Convert to int
	    int in1_int = (int) ((MAX_INT32) * in1);
        int in2_int = (int) ((MAX_INT32) * in2);

        // Apply XOR
        int xorred = (in1_int ^ in2_int);

        // Convert to float and output gain staging
        float xorred_float = 10.f * params[PARAM_GAIN_OUT].getValue() * ((float)xorred / MAX_INT32);

        // Limiter at 0 dB (not sure if needed)
        float output = math::clamp(xorred_float, -5.f, 5.f);
        outputs[SIGNAL_OUTPUT].setVoltage(output);

        // Blink light
        lights[BLINK_LIGHT].setBrightness(abs(output / 5.f));
	}
};


struct CalvarioWidget : ModuleWidget {
		CalvarioWidget(Calvario* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/calvario.svg")));

		//addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(_HP, 11)), module, Calvario::PARAM_GAIN_IN1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 19)), module, Calvario::SIGNAL_GAIN_MOD_IN1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 29)), module, Calvario::SIGNAL_INPUT_IN1));
		
		
		addParam(createParamCentered<Trimpot>(mm2px(Vec(_HP, 41)), module, Calvario::PARAM_GAIN_IN2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 49)), module, Calvario::SIGNAL_GAIN_MOD_IN2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(_HP, 59)), module, Calvario::SIGNAL_INPUT_IN2));
		
		addParam(createParamCentered<CKSS>(mm2px(Vec(_HP, 71)), module, Calvario::GAIN_SWITCH));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(_HP, 85)), module, Calvario::PARAM_GAIN_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(_HP, 95)), module, Calvario::SIGNAL_OUTPUT));
		
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(_HP, .9*_3U+2.4)), module, Calvario::BLINK_LIGHT));
	}
};


Model* modelCalvario = createModel<Calvario, CalvarioWidget>("Calvario");
