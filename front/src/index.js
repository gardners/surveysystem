import React from 'react';
import ReactDOM from 'react-dom';
import registerServiceWorker from './registerServiceWorker';
import 'bootstrap/dist/css/bootstrap.css';

import Navbar from "./components/Navbar";
import FlexibleSurvey from "./components/FlexibleSurvey";
import Page404 from "./components/Page404"
import Welcome from "./components/Welcome"

import { BrowserRouter as Router, Route, Switch } from 'react-router-dom';



//rendering the routes
ReactDOM.render(
    <Navbar />,
    document.getElementById("header"));



ReactDOM.render(
    <Router>
        <Switch>
            <Route exact path="/" component={Welcome} />
            <Route path="/survey/:id" component={FlexibleSurvey} />
            <Route path="*" component={Page404} />
        </Switch>
    </Router>,
    document.getElementById('app')
)




//registerServiceWorker();
