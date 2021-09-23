import { Component } from '@angular/core';

interface Componente{
  icon: string;
  name: string;
  redirecTo:string;

}

@Component({
  selector: 'app-root',
  templateUrl: 'app.component.html',
  styleUrls: ['app.component.scss'],
})
export class AppComponent {
  constructor() {}

  componentes : Componente[] =[
    {
      icon: 'home-outline',
      name: 'inicio', 
      redirecTo: '/inicio'
    },
    {
      icon: 'cart-outline', 
      name: 'Tienda', 
      redirecTo: '/page2'
    },
    {
      icon: 'person-add-outline', 
      name: 'Registro', 
      redirecTo: '/page3'
    },
    
  ]

}