
import * as Survey from "survey-react";

let widget = {
    //the widget name. It should be unique and written in lowcase.
    name: "geolocation",
    //the widget title. It is how it will appear on the toolbox of the SurveyJS Editor/Builder
    title: "geolocation",

    //the name of the icon on the toolbox. We will leave it empty to use the standard one
    iconName: "",
    //If the widgets depends on third-party library(s) then here you may check if this library(s) is loaded
    widgetIsLoaded: function () {
        //return typeof $ == "function" && !!$.fn.select2; //return true if jQuery and select2 widget are loaded on the page
        return true; //we do not require anything so we just return true.
    },
    //SurveyJS library calls this function for every question to check, if it should use this widget instead of default rendering/behavior
    isFit: function (question) {
        //we return true if the type of question is textwithbutton

        return question.getType() === 'geolocation';
        //the following code will activate the widget for a text question with inputType equals to date
        //return question.getType() === 'text' && question.inputType === "date";
    },
    //Use this function to create a new class or add new properties or remove unneeded properties from your widget
    //activatedBy tells how your widget has been activated by: property, type or customType
    //property - it means that it will activated if a property of the existing question type is set to particular value, for example inputType = "date"
    //type - you are changing the behaviour of entire question type. For example render radiogroup question differently, have a fancy radio buttons
    //customType - you are creating a new type, like in our example "textwithbutton"
    activatedByChanged: function (activatedBy) {
        //we do not need to check acticatedBy parameter, since we will use our widget for customType only
        //We are creating a new class and derived it from text question type. It means that text model (properties and fuctions) will be available to us
        Survey.JsonObject.metaData.addClass("geolocation", [], null, "text");

        //Add new property(s)
        //For more information go to https://surveyjs.io/Examples/Builder/?id=addproperties#content-docs
        Survey.JsonObject.metaData.addProperties("geolocation", [
            { name: "buttonText", default: "Get my geolocation" }

        ]);
    },
    //If you want to use the default question rendering then set this property to true. We do not need any default rendering, we will use our our htmlTemplate
    isDefaultRender: false,
    //You should use it if your set the isDefaultRender to false

    htmlTemplate: "<div><input/><button></button></div>",

    //The main function, rendering and two-way binding
    afterRender: function (question, el) {
        //el is our root element in htmlTemplate, is "div" in our case
        //get the text element
        var text = el.getElementsByTagName("input")[0];
        //set some properties
        text.inputType = question.inputType;
        text.placeholder = question.placeHolder;
        //get button and set some properties
        var button = el.getElementsByTagName("button")[0];
        button.innerText = question.buttonText;
        // the next 3 functions are used for the geolocation
        let success = function(pos) {
            let crd = pos.coords;
            console.log('Your position is :');
            console.log(`Latitude : ${crd.latitude}`);
            console.log(`Longitude: ${crd.longitude}`);
            console.log(`With an accuracy of  ${crd.accuracy} meters.`);
            text.value = crd.latitude + ',' + crd.longitude
            question.value = text.value

        }
        let error = function(err) {
            console.warn(`ERROR(${err.code}): ${err.message}`);
        }
        let options = {
            enableHighAccuracy: true,
            timeout: 5000,
            maximumAge: 0
        }

        button.onclick = function (e) {
            e.preventDefault();
            navigator.geolocation.getCurrentPosition(success, error, options);

        }

        //set the changed value into question value
        text.onchange = function () {
            question.value = text.value;
        }

        let onValueChangedCallback = function () {
            text.value = question.value ? question.value : "";
        }
        let onReadOnlyChangedCallback = function () {

            if (question.isReadOnly) {
                text.setAttribute('disabled', 'disabled');
                button.setAttribute('disabled', 'disabled');
            } else {
                text.removeAttribute("disabled");
                button.removeAttribute("disabled");
            }
        };
        //if question becomes readonly/enabled add/remove disabled attribute
        question.readOnlyChangedCallback = onReadOnlyChangedCallback;
        //if the question value changed in the code, for example you have changed it in JavaScript
        question.valueChangedCallback = onValueChangedCallback;
        //set initial value
        onValueChangedCallback();
        //make elements disabled if needed
        onReadOnlyChangedCallback();

    }
}

export default widget;
