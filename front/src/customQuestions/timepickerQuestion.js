const timepicker = {
    name: 'timepicker',
    title: 'time picker',
    iconName: '',
    //load widget icon and name at left panel
    widgetIsLoaded: function() {
        return !!$ && !!$.fn.timepicker;
    },
    isFit: function(question) {
        return question.getType() === 'timepicker';
    },
    //Append control type of widget
    htmlTemplate: "<input class='form-control widget-timepicker' type='text'>",
    afterRender: function(question, el) {
        const $el = $(el).is('.widget-timepicker') ? $(el) : $(el).find('.widget-timepicker');
        const config = question.config || {};
        const pickerWidget = $el.timepicker(config);

        //set time on question.value changed
        const updateValueHandler = function() {
            pickerWidget.timepicker('update', question.value);

        };
        if (question.isReadOnly) {
            el.setAttribute('disabled', true);
        }
        //Update question value on value change
        pickerWidget.on('changeTime.timepicker', (e) => {
            question.value = e.time.value;
        });
        //set initial data
        updateValueHandler();
        question.valueChangedCallback = updateValueHandler;
        question.readOnlyChangedCallback = function() {
            if (question.isReadOnly) {
                el.setAttribute('disabled', true);
            } else {
                el.removeAttribute('disabled');
            }
        };
    },
    activatedByChanged: function(activatedBy) {
        Survey.JsonObject.metaData.addClass(
            'timepicker', [{
                name: 'inputType',
                visible: false
            }, {
                name: 'inputFormat',
                visible: false
            }, {
                name: 'inputMask',
                visible: false
            }],
            null,
            'text'
        );
        Survey.JsonObject.metaData.addProperty('timepicker', {
            pickTime: false
        });


    },

    willUnmount: function(question, el) {
        const $el = $(el).is('.widget-timepicker') ? $(el) : $(el).find('.widget-timepicker');
        $el.timepicker('destroy');
    }

};


export default timepicker;
